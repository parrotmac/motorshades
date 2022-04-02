.PHONY: build
build: compile upload

.PHONY: compile
compile:
	arduino-cli compile -b esp8266:esp8266:nodemcuv2 .

.PHONY: upload
upload:
	arduino-cli upload . --protocol serial -b esp8266:esp8266:nodemcuv2 --port /dev/cu.usbserial-2110


.PHONY: deps
deps:
	echo "May want to run 'arduino-cli config init' if you haven't already"
	arduino-cli core update-index --additional-urls https://arduino.esp8266.com/stable/package_esp8266com_index.json
	arduino-cli core install esp8266:esp8266 --additional-urls https://arduino.esp8266.com/stable/package_esp8266com_index.json
	arduino-cli lib install ArduinoJson
	arduino-cli lib install EspMQTTClient

.PHONY: install_arduino_cli
install_arduino_cli:
	# Note! As of 2022-04-02, this doesn't work form arm64 macOS or Linux
	curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh

