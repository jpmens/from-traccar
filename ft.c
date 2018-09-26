#include <stdio.h>
#include <mosquitto.h>
#include <syslog.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <microhttpd.h>
#include "utstring.h"
#include <time.h>
#include <assert.h>
#include "json.h"
#include <statsd/statsd-client.h>

/*
 * ft := from Traccar Traccar posts positions and events. We extract the JSON
 * from the body and publish it via MQTT.
 */

#define S_PORT 	"8840"
#define POSTBUFFERSIZE 65535

struct connection_info_struct {
	UT_string *ds;
	size_t size;
};

static struct statistics {
	time_t launchtime;
	unsigned long requests;
	unsigned long positions;
	unsigned long events;
	unsigned long stats;
} st;

#define SAMPLE_RATE	1.0
statsd_link *sd;

#define DEBUG 1

#define TOPIC_BRANCH	"_traccar/"
#define PREFIX		"owntracks/qtripp/"	/* strip this to get uniqueId */
struct mosquitto *mosq;

static char *mosquitto_reason(int rc)
{
	static char *reasons[] = {
		"Connection accepted",					/* 0x00 */
		"Connection refused: incorrect protocol version",	/* 0x01 */
		"Connection refused: invalid client identifier",	/* 0x02 */
		"Connection refused: server unavailable",		/* 0x03 */
		"Connection refused: code=0x04",			/* 0x04 */
		"Connection refused: bad username or password",		/* 0x05 */
		"Connection refused: not authorized",			/* 0x06 */
		"Connection refused: TLS error",			/* 0x07 */
	};

	return ((rc >= 0 && rc <= 0x07) ? reasons[rc] : "unknown reason");
}


void on_disconnect(struct mosquitto *mosq, void *userdata, int reason)
{
	if (reason == 0) { //client wish
		;
	} else {
		syslog(LOG_INFO, "Disconnected. Reason: 0x%X [%s]", reason, mosquitto_reason(reason));
	}
}

static int send_content(struct MHD_Connection *conn, const char *page,
		const char *content_type, int status_code)
{
	int ret;
	struct MHD_Response *response;

	response = MHD_create_response_from_buffer(strlen(page),
				(void*) page, MHD_RESPMEM_MUST_COPY);
	if (!response)
		return (MHD_NO);
	MHD_add_response_header(response, "Content-Type", content_type);
	ret = MHD_queue_response(conn, status_code, response);
	MHD_destroy_response(response);
	return (ret);
}

static int get_stats(struct MHD_Connection *connection)
{
	JsonNode *json = json_mkobject(), *counters = json_mkobject();
	char *js;

	assert(json && counters);

	json_append_member(counters, "_whoami",		json_mkstring(__FILE__));
	json_append_member(counters, "_tst",		json_mknumber(time(0)));
	json_append_member(counters, "stats",		json_mknumber(++st.stats));
	json_append_member(counters, "requests",	json_mknumber(st.requests));
	json_append_member(counters, "positions",	json_mknumber(st.positions));
	json_append_member(counters, "events",		json_mknumber(st.events));

	json_append_member(json, "stats", counters);
	json_append_member(json, "uptime", json_mknumber(time(0) - st.launchtime));

	if ((js = json_stringify(json, NULL)) != NULL) {
		int ret = send_content(connection, js, "application/json", MHD_HTTP_OK);
		free(js);
		json_delete(json);
		return (ret);
	}

	json_delete(json);

	return send_content(connection, "{}", "application/json", MHD_HTTP_OK);

}



void publish(const char *json_string)
{
#ifdef DEBUG
	FILE *fp = fopen("/tmp/ft.json", "a");
#endif
	char *s, *uniqueid = NULL, *event = NULL;
	char *type = "position", s_type[128];
	JsonNode *d, *e, *j, *json;
	static UT_string *mtopic;
	int rc;

	utstring_renew(mtopic);

	utstring_printf(mtopic, "%s", TOPIC_BRANCH);

	st.requests++;

	if ((json = json_decode(json_string)) == NULL) {
		puts("meh");
		puts(json_string);
		statsd_inc(sd, "bad.payloads", SAMPLE_RATE);
		return;
	}

	if ((d = json_find_member(json, "device")) != NULL) {
		if ((j = json_find_member(d, "uniqueId")) != NULL) {
			if (j->tag == JSON_STRING) {
				uniqueid = j->string_;
			}
		}
	}

	if ( (json_find_member(json, "position") == NULL) &&
		(json_find_member(json, "event") == NULL) )
	{
		syslog(LOG_INFO, "Neither position nor event; is this a Traccar payload?");
		json_delete(json);
		return;
	}

	if (uniqueid == NULL) {
		syslog(LOG_INFO, "No uniqueid; is this a Traccar payload?");
		json_delete(json);
		return;
	}

	uniqueid = (uniqueid) ? uniqueid : "<unknown>";
	if (strncmp(uniqueid, PREFIX, strlen(PREFIX)) == 0) {
		uniqueid = uniqueid + strlen(PREFIX);
	}
	utstring_printf(mtopic, "%s/", uniqueid);

	if ((e = json_find_member(json, "event")) != NULL) {
		if ((j = json_find_member(e, "type")) != NULL) {
			if (j->tag == JSON_STRING) {
				type = "event";
				event = j->string_;

			}
		}
	}

	snprintf(s_type, sizeof(s_type), "requests.%s", type);
	statsd_inc(sd, s_type, SAMPLE_RATE);

	utstring_printf(mtopic, "%s", type);
	if (event) {
		utstring_printf(mtopic, "/%s", event);

		snprintf(s_type, sizeof(s_type), "event.%s", event);
		st.events++;
		statsd_inc(sd, s_type, SAMPLE_RATE);
	} else {
		st.positions++;
	}

	fprintf(stderr, "%s\n", utstring_body(mtopic));

#if DEBUG
	if ((s = json_stringify(json, "  ")) != NULL) {
		//puts(s);
		if (fp != NULL) {
			fprintf(fp, "%s\n", s);
			fclose(fp);
		}
		free(s);
	}
#endif /* DEBUG */

	/*
	 * Stringify the JSON instead of relying on what we get from Traccar
	 * ... Experience shows that upstream often changes stuff, and I
	 * don't want to be surprised with newlines in the payload.
	 */

	if ((s = json_stringify(json, NULL)) != NULL) {
		rc = mosquitto_publish(mosq, NULL, utstring_body(mtopic),
				 strlen(s), s, 1, false);
		free(s);
		statsd_inc(sd, "requests.tomqtt", SAMPLE_RATE);
	}
	json_delete(json);
}


void request_completed(void *cls, struct MHD_Connection *connection,
		            void **con_cls,
		            enum MHD_RequestTerminationCode toe)
{
	struct connection_info_struct *con_info = *con_cls;

	if (!con_cls || !*con_cls) {
		/* For example on a POST to an incorrect URL */
		return;
	}

	if (con_info->ds)
		utstring_free(con_info->ds);

	free(con_info);
	*con_cls = NULL;
}

bool process_data(struct MHD_Connection *connection, void **con_cls)
{
	struct connection_info_struct *con_info = *con_cls;
	char *request_body = utstring_body(con_info->ds);

	publish(request_body);

	return true;
}

static int on_client_connect(void *cls, const struct sockaddr *addr, socklen_t addrlen)
{
	return MHD_YES;
}

int print_kv(void *cls, enum MHD_ValueKind kind, const char *key, const char *value)
{
	fprintf(stderr, "%s: %s\n", key, value);
	return (MHD_YES);
}

static int send_page(struct MHD_Connection *connection, const char *page, int status_code)
{
	return send_content(connection, page, "text/plain", status_code);
}

int handle_connection(void *cls, struct MHD_Connection *connection,
	const char *url,
	const char *method, const char *version,
	const char *upload_data,
	size_t *upload_data_size, void **con_cls)
{
	struct connection_info_struct *con_info = (struct connection_info_struct *)*con_cls;

	if ((strcmp(method, "GET") == 0) && (strcmp(url, "/stats") == 0)) {
		return get_stats(connection);
	}

	if (strcmp(method, "POST") != 0) {
		return (send_page(connection, "Go away!", MHD_HTTP_METHOD_NOT_ALLOWED));
	}

	if (strcmp(url, "/evpos") != 0) {
		return (send_page(connection, "not found", MHD_HTTP_NOT_FOUND));
	}


	if (*con_cls == NULL) {
		/* don't respond on first call, but set up post processor */

		struct connection_info_struct *con_info;
		struct sockaddr *so;
		char ip[128];



		so = MHD_get_connection_info(connection, MHD_CONNECTION_INFO_CLIENT_ADDRESS)->client_addr;

		if (so->sa_family == AF_INET) {
			struct sockaddr_in *v4 = (struct sockaddr_in *)so;
			inet_ntop(AF_INET, &(v4->sin_addr), ip, 20);
		}
		if (so->sa_family == AF_INET6) {
			struct sockaddr_in6 *v6 = (struct sockaddr_in6 *)so;
			inet_ntop(AF_INET6, &(v6->sin6_addr), ip, 20);
		}

		fprintf(stderr, "Client %s: %s %s %s\n", ip, method, url, version);
		MHD_get_connection_values(connection, MHD_HEADER_KIND, &print_kv, NULL);

		if ((con_info = malloc(sizeof (struct connection_info_struct))) == NULL) {
			return (MHD_NO);
		}
		utstring_new(con_info->ds);
		con_info->size = 0;

		*con_cls = (void *)con_info;
		return (MHD_YES);
	}

	if (*upload_data_size) {

		// fprintf(stderr, "--> handling %zu... bytes\n", *upload_data_size);

		con_info->size += *upload_data_size;
		utstring_bincpy(con_info->ds, upload_data, *upload_data_size);

		// MHD_post_process(con_info->postprocessor, upload_data, *upload_data_size);
		*upload_data_size = 0;
		return (MHD_YES);
	}

	/*
	 * We've received an HTTP request from Traccar with a
	 * position or an event in it, and their upload of the
	 * data is now complete; we can process the data.
	 */

	fprintf(stderr, "++++++ Here we are with a total of: %zu bytes\n\n", con_info->size);
	fprintf(stderr, "++++++ %s\n\n", utstring_body(con_info->ds));

	statsd_inc(sd, "requests.incoming", SAMPLE_RATE);
	statsd_gauge(sd, "requests.incoming.size", con_info->size);

	process_data(connection, con_cls);

	return (send_page(connection, "", MHD_HTTP_OK));
}





int main(int argc, char **argv)
{
	struct MHD_Daemon *daemon;
	struct sockaddr_in sad;
	char *s_ip, *s_port;
	unsigned short port;
	char *mqtt_hostname, *mqtt_sport;
	char *ftuser = NULL, *ftpass = NULL;
	int run = true, rc, mqtt_port = 1883;
	int loop_timeout = 5000;

	if ((s_ip = getenv("FT_IP")) == NULL)
		s_ip = "127.0.0.1";
	if ((s_port = getenv("FT_PORT")) == NULL)
		s_port = S_PORT;

	if ((mqtt_hostname = getenv("FT_MQTT_HOST")) == NULL)
		mqtt_hostname = "127.0.0.1";
	if ((mqtt_sport = getenv("FT_MQTT_PORT")) == NULL)
		mqtt_sport = "1883";
	mqtt_port = atoi(mqtt_sport);

	port = atoi(s_port);

	memset(&sad, 0, sizeof (struct sockaddr_in));
	sad.sin_family = AF_INET;
	sad.sin_port = htons(port);
	if (inet_pton(AF_INET, s_ip, &(sad.sin_addr.s_addr)) != 1) {
		perror("Can't parse IP");
		exit(2);
	}

	openlog("from-traccar", LOG_PERROR, LOG_DAEMON);
	st.launchtime = time(0);

	mosquitto_lib_init();

	mosq = mosquitto_new("from-traccar", true, NULL);
	if (!mosq) {
		fprintf(stderr, "Error: Out of memory.\n");
		mosquitto_lib_cleanup();
		return (1);
	}
	mosquitto_reconnect_delay_set(mosq,
				      2,	/* delay */
				      20,	/* delay_max */
				      0);	/* exponential backoff */

	mosquitto_disconnect_callback_set(mosq, on_disconnect);

	ftuser = getenv("FT_USER");
	ftpass = getenv("FT_PASS");
	if (ftuser || ftpass) {
		mosquitto_username_pw_set(mosq, ftuser, ftpass);
	}

	rc = mosquitto_connect(mosq, mqtt_hostname, mqtt_port, 60);
	if (rc) {
		char err[1024];

		if (rc == MOSQ_ERR_ERRNO) {
			strerror_r(errno, err, 1024);
			fprintf(stderr, "Error: %s\n", err);
		} else {
			fprintf(stderr, "Unable to connect (%d) [%s]: %s.\n",
			  rc, mosquitto_strerror(rc), mosquitto_reason(rc));
		}
		mosquitto_lib_cleanup();
		return (rc);
	}

	sd = statsd_init_with_namespace(STATSDHOST, 8125, "fromtraccar");

	daemon = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY, 0, on_client_connect, NULL,
		&handle_connection, NULL,
		MHD_OPTION_NOTIFY_COMPLETED, &request_completed, NULL,
		MHD_OPTION_SOCK_ADDR, &sad,
		MHD_OPTION_END);

	if (daemon == NULL) {
		perror("heh");
		return (1);
	}

	while (run) {
		rc = mosquitto_loop(mosq, loop_timeout, /* max-packets */ 1);
		if (run && rc) {
			syslog(LOG_INFO, "MQTT connection: rc=%d [%s] (errno=%d; %s). Sleeping...", rc, mosquitto_strerror(rc), errno, strerror(errno));
			sleep(10);
			mosquitto_reconnect(mosq);
		}
		// fprintf(stderr, "In loop ...\n");
		sleep(1);
	}

	MHD_stop_daemon(daemon);
	statsd_finalize(sd);

	return (0);
}
