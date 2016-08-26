# v2.0.0-rc.8 - Release Candidate 8

### Bug Fixes

* Removed `ackCounter` from `Device`
* Removed `ruby` and `python` test files for cleaning

# v2.0.0-rc.7 - Release Candidate 7

### Bug Fixes

* Increased time it takes to time out a serial page write to 0.5ms from 5ms

# v2.0.0-rc.6 - Release Candidate 6

### New Features

* New baud rate added, `Hyper` mode at a screaming `921600` baud! Get those packets off the serial port faster than ever!

### Bug Fixes

* Addressed problem of packet loss over distance with 20 packet stream packet ring of buffers!
* Increased time it takes to time out a serial page write to 5ms from 3ms

### Work in progress

* Addresses issue of `Read Page Failure` on verify of over the air programming.

# v2.0.0-rc.5 - Release Candidate 5

### Bug Fixes

* Fixed bug where Serial buffer was not processed on comms down. Added over 50 auto tests to ensure proper operation.

### Breaking Changes

* Renamed `OPENBCI_MAX_NUMBER_OF_BUFFERS` to `OPENBCI_NUMBER_SERIAL_BUFFERS` to match new convention.
* Renamed `processCommsFailure` with `bufferSerialProcessCommsFailure` to follow new convention.

# v2.0.0-rc.4 - Release Candidate 4

### Enhancements

* Refactor of the was the device reads and processes chars. Completely auto tested.
* Complete auto test coverage over mission critical buffer functions.
* Less packet loss over same distance as compared with previous release candidates.
* Renamed a ton of functions to follow a new convention where there are three main buffer categories:
  * `Serial` which handles non streaming data coming from serial port to radio
  * `Radio` which handles non streaming data coming from radio to serial port
  * `Stream` which handles both streaming data in both directions.

### Breaking Changes

* Removed function `isAStreamPacketWaitingForLaunch`. Now just check if stream packet buffer is in the ready state.
* Renamed `thereIsDataInSerialBuffer` to `bufferSerialHasData`
* Renamed `packetsInSerialBuffer` for `bufferSerialHasData` to follow new convention.
* Renamed `sendStreamPacketToTheHost` to `bufferStreamSendToHost` to follow new convention.
* Renamed `storeCharToSerialBuffer` to `bufferSerialAddChar` to follow new convention.
* Refactored `processSerialCharDevice` into `bufferSerialAddChar` and `bufferStreamAddChar`.
* Removed ring buffer.

# v2.0.0-rc.3 - Release Candidate 3

### Enhancements

* Completely redesigned the radio buffer and the stream buffers. Completely automatically tested, 100% code coverage on these sections.

### Breaking Changes

* Over the air programming has slowed down from a couple days ago, PTW is reportedly seeing speeds around 750 bytes/sec. However the stability is impressive and worth the drop in speed. Please turn your poll times up to 50 from 80.

### Work In Progress (WIP)

* Addresses issue of `Read Page Failure` on verify of over the air programming.
* Addresses issue of packet loss over distance; still work in progress.


# v2.0.0-rc.2 - Release Candidate 2

### Enhancements

* Add verification test scripts for time syncing

### New Features

* Add private radio command for controlling GPIO 2

# v2.0.0-rc.1 - Release Candidate 1

### Enhancements

* Add multiple stream packet buffers on the Host code to allow for a better transfer to the ring buffer.

### Bug Fixes

* Fixes several bugs with private radio commands
* Removed line of code that dumped the ring buffer on overflow, now packets are not added. Prevents major loss of packets.

# v2.0.0-rc.0 - Release Candidate 0

### New Features
* Bumped beta to release candidate

# v2.0.0-beta.0 - Beta

Introducing firmware version 2 (`v`)!

* Improved stability.
* Faster and stable over the air programming
* Change radio channels from the driver/PC
* Change the poll time from the driver/PC (allows older computers to do Over The Air programming)
* Get system statuses from simple commands
* Stateless! Can't get stuck in a state.
