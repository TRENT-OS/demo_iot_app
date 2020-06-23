# Demo IoT App

Demo application that showcases a rather typical IoT use case. A sensor
component cyclically publishes a temperature update via MQTT to a
cloudConnector component. This component then opens up a TLS-Connection to an
MQTT broker and in turn publishes the MQTT message to it. The configuration for
the nwStack (network stack), the sensor and the cloudConnector component can be
set in the "demo_iot_default_config.xml" file. When running the demo, the
nwStack, sensor and cloudConnector component will all contact the configServer
and retrieve their required configuration parameters through it. Once all
components have initialized, the sensor will proceed to contact the
cloudConnector about every 5 seconds to send a message to the configured cloud
service. The already included default XML configuration file is set to connect
to a Mosquitto MQTT broker running inside the test container.

## Run

To run this demo, just call the "run_demo.sh" script found in this repository
from inside the test container and pass it the build directory of the demo and
the path to the proxy application.

    ./run_demo.sh <path-to-project-build> <path-to-proxy>