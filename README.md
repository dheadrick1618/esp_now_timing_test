ESP NOW timing test program
====================

This program consists of two esp idf projects, a broadcaster, and a listener. The goal is to measure an approximate time between the time an esp now packet is broadcast, and the time elapsed until the listener receives and processes the broadcasted packet. Both programs are kept simple to be remain easy to understand.

The broadcaster will set a GPIO output high following each time an esp packeet is broadcast. The receiver will read this via a GPIO input, trigger an interrupt, and record the system clock time elapsed until the broadcast packet is recevied entirely. The total elapsed time will then be calculated based on the system clock frequency of the esp32.

## TODO

- Measure the response time of packets of various size
- Measure the response time of packets over various distances

## License

*Code in this repository is in the Public Domain (or CC0 licensed, at your option.)
Unless required by applicable law or agreed to in writing, this
software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
CONDITIONS OF ANY KIND, either express or implied.*