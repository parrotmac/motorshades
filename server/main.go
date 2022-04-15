package main

import (
	"context"
	"encoding/json"
	"fmt"
	"log"
	"net/http"
	"net/url"
	"os"
	"strconv"
	"strings"
	"time"

	"github.com/parrotmac/nicemqtt"
)

type server struct {
	mqtt nicemqtt.Client
}

type adjustmentMessage struct {
	Pin       int32 `json:"pin"`
	Duration  int32 `json:"duration"`
	DutyCycle int32 `json:"dutyCycle"`
}

func (s *server) sendAdjustment(ctx context.Context, target string, direction string, duration int32, dutyCycle int32) error {
	var pin int32
	switch direction {
	case "forward", "up", "plus", "a":
		pin = 1
	case "backward", "down", "minus", "b":
		pin = 2
	default:
		return fmt.Errorf("invalid direction: %s", direction)
	}

	if duration < 0 || duration > 2000 {
		return fmt.Errorf("invalid duration: %d", duration)
	}

	if dutyCycle < 0 || dutyCycle > 255 {
		return fmt.Errorf("invalid dutyCycle: %d", dutyCycle)
	}

	msg := adjustmentMessage{
		Pin:       pin,
		Duration:  duration,
		DutyCycle: dutyCycle,
	}

	msgBody, err := json.Marshal(msg)
	if err != nil {
		return err
	}

	return s.mqtt.Publish(ctx, fmt.Sprintf("%s/position/json", target), 1, false, msgBody)
}

func (s *server) adjustHandler(w http.ResponseWriter, r *http.Request) {
	q := r.URL.Query()
	rawTarget := q.Get("target")
	direction := q.Get("direction")
	durationStr := q.Get("duration")
	dutyCycleStr := q.Get("dutyCycle")

	duration, err := strconv.Atoi(durationStr)
	if err != nil {
		http.Error(w, err.Error(), http.StatusBadRequest)
		return
	}

	dutyCycle, err := strconv.Atoi(dutyCycleStr)
	if err != nil {
		http.Error(w, err.Error(), http.StatusBadRequest)
		return
	}

	// Remove potentially problematic characters
	mqttReplacer := strings.NewReplacer("/", "_", "+", "_", "#", "_")
	target := mqttReplacer.Replace(rawTarget)

	err = s.sendAdjustment(r.Context(), target, direction, int32(duration), int32(dutyCycle))
	if err != nil {
		http.Error(w, err.Error(), http.StatusInternalServerError)
		return
	}

	w.WriteHeader(http.StatusOK)
}

func main() {
	mqttURL, err := url.Parse(os.Getenv("MQTT_URL"))
	if err != nil {
		log.Panicln(err)
	}
	port := 1883
	if mqttURL.Port() != "" {
		port, err = strconv.Atoi(mqttURL.Port())
		if err != nil {
			log.Panicln(err)
		}
	}
	clientID := os.Getenv("CLIENT_ID")

	log.Printf("Connecting to MQTT broker at %s:%d as %s", mqttURL.Hostname(), port, clientID)
	mqttClient, err := nicemqtt.NewClient(mqttURL.Hostname(), port, clientID)
	if err != nil {
		log.Panicln(err)
	}

	connectCtx, cancel := context.WithTimeout(context.Background(), time.Minute)
	defer cancel()
	err = mqttClient.Connect(connectCtx)
	if err != nil {
		log.Panicln(err)
	}

	srv := &server{
		mqtt: mqttClient,
	}

	mux := http.NewServeMux()

	mux.HandleFunc("/adjust", srv.adjustHandler)

	bindAddr := ":8080"
	envBindAddr := os.Getenv("BIND_ADDR")
	if envBindAddr != "" {
		bindAddr = envBindAddr
	}

	log.Printf("Listening on %s\n", bindAddr)

	s := &http.Server{
		Addr:    bindAddr,
		Handler: mux,
	}
	log.Println(s.ListenAndServe())
}
