curl -v \
	--header "Content-Type: application/json" \
	--data @p.json \
	http://127.0.0.1:8840/evpos

netstat -an|grep -c "8840.*ESTABLISHED"
