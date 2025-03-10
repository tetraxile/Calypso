# Calypso

## Server

### Prerequisites

`pip install websockets>=14.0`

### Running the server

`python3 server/run.py`
open `http://localhost:8172/` in a browser

## Setup

### Building

Follow the instructions listed in [LibHakkun's README](https://github.com/fruityloops1/LibHakkun?tab=readme-ov-file#setup).

### Deploying

copy the `romfs/` directory directly to `/atmosphere/contents/0100000000010000` on your switch

## License

The licenses found in the [LICENSE](LICENSE) file apply only to the source files in the [src/](src) and [server/](server) directories.
This project makes use of [LibHakkun](https://github.com/fruityloops1/LibHakkun), whose license can be found [here](sys/LICENSE), as well as [OdysseyHeaders](https://github.com/MonsterDruide1/OdysseyHeaders).
