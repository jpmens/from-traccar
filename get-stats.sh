#!/bin/sh

exec curl -s http://localhost:8840/stats | jq . 
