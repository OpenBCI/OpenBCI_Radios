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
