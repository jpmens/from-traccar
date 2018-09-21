#include <stdio.h>
#include <mosquitto.h>
#include <syslog.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include "mongoose.h"
#include "json.h"

// #define DEBUG 1

/*
 * ft := from Traccar Traccar posts positions and events. We extract the JSON
 * from the body and publish it via MQTT.
 */

static const char *s_http_port = "127.0.0.1:8840";

#define TOPIC_BRANCH	"_traccar/"
#define PREFIX		"owntracks/qtripp/"	/* strip this to get uniqueId */
struct mbuf mtopic;
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

void publish(const char *json_string)
{
#ifdef DEBUG
	FILE *fp = fopen("/tmp/ft.json", "a");
#endif
	char *s, *uniqueid = NULL, *event = NULL;
	char *type = "position";
	JsonNode *d, *e, *j, *json;

	mtopic.len = 0;
	mbuf_append(&mtopic, TOPIC_BRANCH, strlen(TOPIC_BRANCH));

	if ((json = json_decode(json_string)) == NULL) {
		puts("meh");
		puts(json_string);
		return;
	}
	if ((d = json_find_member(json, "device")) != NULL) {
		if ((j = json_find_member(d, "uniqueId")) != NULL) {
			if (j->tag == JSON_STRING) {
				uniqueid = j->string_;
			}
		}
	}
	uniqueid = (uniqueid) ? uniqueid : "<unknown>";
	if (strncmp(uniqueid, PREFIX, strlen(PREFIX)) == 0) {
		uniqueid = uniqueid + strlen(PREFIX);
	}
	mbuf_append(&mtopic, uniqueid, strlen(uniqueid));
	mbuf_append(&mtopic, "/", 1);


	if ((e = json_find_member(json, "event")) != NULL) {
		if ((j = json_find_member(e, "type")) != NULL) {
			if (j->tag == JSON_STRING) {
				type = "event";
				event = j->string_;
			}
		}
	}

	mbuf_append(&mtopic, type, strlen(type));
	if (event) {
		mbuf_append(&mtopic, "/", 1);
		mbuf_append(&mtopic, event, strlen(event));
	}

	mbuf_append(&mtopic, "\0", 1);
	fprintf(stderr, "%s\n", mtopic.buf);

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
		mosquitto_publish(mosq, NULL, mtopic.buf,
				 strlen(s), s, 1, false);
		free(s);
	}
	json_delete(json);
}


static void ev_handler(struct mg_connection *c, int ev, void *p)
{
	struct mbuf *mb = (struct mbuf *)c->mgr->user_data;


	if (ev == MG_EV_HTTP_REQUEST) {
		struct http_message *hm = (struct http_message *)p;

		mb->len = 0;
		mbuf_append(mb, hm->body.p, hm->body.len);
		mbuf_append(mb, "\0", 1);

		if (mg_vcmp(&hm->uri, "/evpos") == 0) {

			/*
			 * We've received an HTTP request from Traccar with a
			 * position or en event in it. Grab the JSON from the
			 * request body.
			 */

			//hm->body.p[hm->body.len] = 0;
			//printf("BODY: [%.*s]\n", (int)hm->body.len, hm->body.p);

			publish(mb->buf);

			mg_send_head(c, 200, 0, "Content-Type: text/plain\nConnection: keep-alive");
			// mg_printf_http_chunk(c, "%s", res);
			// mg_send_http_chunk(c, "", 0); // Tell the client we're finished
			// c->flags |= MG_F_SEND_AND_CLOSE;

		} else {
			char *res = "go away";

			mg_send_head(c, 404, strlen(res), "Content-Type: text/plain");
			mg_printf(c, "%s", res);
			c->flags |= MG_F_SEND_AND_CLOSE;
		}
	}
}

int main(int argc, char **argv)
{
	struct mg_mgr mgr;
	struct mg_connection *c;
	struct mbuf mb;
	char *hostname = "127.0.0.1";
	char *ftuser = NULL, *ftpass = NULL;
	int run = true, rc, port = 1883;
	int loop_timeout = 10;

	openlog("from-traccar", LOG_PERROR, LOG_DAEMON);

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

	rc = mosquitto_connect(mosq, hostname, port, 60);
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
	mbuf_init(&mb, 2048);
	mg_mgr_init(&mgr, &mb);

	c = mg_bind(&mgr, s_http_port, ev_handler);
	if (c == NULL) {
		perror("cannot bind http manager");
		exit(2);
	}
	mg_set_protocol_http_websocket(c);

	mbuf_init(&mtopic, 2048);

	while (run) {
		mg_mgr_poll(&mgr, 9000);

		rc = mosquitto_loop(mosq, loop_timeout, /* max-packets */ 1);
		if (run && rc) {
			syslog(LOG_INFO, "MQTT connection: rc=%d [%s] (errno=%d; %s). Sleeping...", rc, mosquitto_strerror(rc), errno, strerror(errno));
			sleep(10);
			mosquitto_reconnect(mosq);


		}
	}
	mg_mgr_free(&mgr);

	return 0;
}
