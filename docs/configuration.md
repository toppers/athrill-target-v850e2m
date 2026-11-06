# V850 target configuration

This page covers parameters read by the Athrill V850E2M target. Command-line options, the memory configuration, the
device-configuration grammar, and common parameters such as CPU frequency, clock skipping, debugger logging, and
external devices are in the
[Athrill core configuration reference](https://github.com/toppers/athrill/blob/master/docs/configuration.md).

The target models one V850E2M core. The checked-in CMake build uses the 32-bit TAAn timer model, external devices,
and a minimal peripheral set by default. mROS is optional, while the retained VDEV sources are used only by
compatible builds that include them. Paths are resolved from the process working directory.

## Timer

| Key | Meaning and accepted values | Default and dependency | Example line |
| --- | --- | --- | --- |
| `DEVICE_CONFIG_TIMER_FD` | Positive decimal number of virtual CPU clocks per TAAn timer count. Compare scheduling uses the same factor. | `100` in the 32-bit timer used by the CMake build. Do not use zero: the timer divides and takes a remainder by this value. A build with VDEV also uses the value as its clocks-per-microsecond conversion factor. | `DEVICE_CONFIG_TIMER_FD 100` |

## Serial file channel

UD0 uses Athrill's terminal/debugger stream. `SERIAL_FILE_PATH` switches UD1 to a pair of files: guest output is
appended to `serial_in.txt`, and bytes appended by the host to `serial_out.txt` after it is first opened become guest
input. The directory and the input file must exist when the target first opens them.

The checked-in CMake build defines `DISABLE_SERIAL_CLOCK` and `MINIMUM_DEVICE_CONFIG`. It therefore does not poll
serial input or run the UD1 cache service; builds that use the file channel must service serial clocks and include
UD1 in `device_supply_clock_serial`.

| Key | Meaning and accepted values | Default and dependency | Example line |
| --- | --- | --- | --- |
| `SERIAL_FILE_PATH` | Directory containing `serial_in.txt` and `serial_out.txt`. It selects file operations for UD1; UD0 is unchanged. | Absent: UD1 uses the normal debug serial stream instead of files. Requires UD1 serial-clock service for input polling and periodic output-cache service. | `SERIAL_FILE_PATH serial-files` |
| `DEBUG_FUNC_SERIAL_FLUSH_COUNT_MAX` | Decimal number of UD1 service calls allowed before the target asks the file cache to flush output and load appended input. Zero asks on every service call. | `100`. Used only by the UD1 file operations selected with `SERIAL_FILE_PATH`; the file cache may defer small writes further. | `DEBUG_FUNC_SERIAL_FLUSH_COUNT_MAX 100` |

## Legacy mROS CAN bridge

These keys are read by a POSIX build configured with `ATHRILL_ENABLE_MROS=ON`. The option is off by default. The
core page documents the mROS master, node, and port keys used by the underlying transport. Topic indices are
zero-based; each configured publish or subscribe entry corresponds to a virtual CAN mailbox in index order.

| Key | Meaning and accepted values | Default and dependency | Example line |
| --- | --- | --- | --- |
| `DEBUG_FUNC_ENABLE_MROS` | Nonzero initializes the virtual CAN peripheral and starts its mROS publishers and subscribers. | `0`. Requires a build with mROS and its transport enabled. | `DEBUG_FUNC_ENABLE_MROS 1` |
| `DEBUG_FUNC_MROS_TOPIC_PUB_NUM` | Non-negative decimal number of publish topics. | Absent: zero publish topics. Read only when mROS is enabled at build time and `DEBUG_FUNC_ENABLE_MROS` is nonzero. | `DEBUG_FUNC_MROS_TOPIC_PUB_NUM 1` |
| `DEBUG_FUNC_MROS_TOPIC_PUB_<n>` | Topic name for zero-based publish mailbox `n`. Device-configuration strings cannot contain whitespace, commas, or colons. | Required for every index from zero through `DEBUG_FUNC_MROS_TOPIC_PUB_NUM - 1`. | `DEBUG_FUNC_MROS_TOPIC_PUB_0 channel0/CAN_IDE0_RTR0_DLC8_0x001` |
| `DEBUG_FUNC_MROS_TOPIC_SUB_NUM` | Non-negative decimal number of subscription topics. | Absent: zero subscription topics. Read only when mROS is enabled at build time and `DEBUG_FUNC_ENABLE_MROS` is nonzero. | `DEBUG_FUNC_MROS_TOPIC_SUB_NUM 1` |
| `DEBUG_FUNC_MROS_TOPIC_SUB_<n>` | Topic name for zero-based subscription mailbox `n`. An incoming message is copied to that mailbox's guest buffer and raises its receive flag. | Required for every index from zero through `DEBUG_FUNC_MROS_TOPIC_SUB_NUM - 1`. | `DEBUG_FUNC_MROS_TOPIC_SUB_0 channel0/CAN_IDE0_RTR0_DLC8_0x123` |

## Legacy VDEV synchronization

These parameters belong to the retained VDEV peripheral. The checked-in CMake configuration excludes the VDEV
sources; they are read by compatible builds that compile those sources and expose the VDEV memory region. The
peripheral can exchange its 1 KiB synchronization packet over UDP or through two buffers in guest RAM.

### Mode selection

| Key | Meaning and accepted values | Default and dependency | Example line |
| --- | --- | --- | --- |
| `DEBUG_FUNC_ENABLE_VDEV` | Nonzero initializes VDEV and supplies it with virtual clocks. | `0`. Requires a build containing the VDEV peripheral. | `DEBUG_FUNC_ENABLE_VDEV 1` |
| `DEBUG_FUNC_VDEV_SIMSYNC_TYPE` | A string beginning with `MMAP` selects guest-RAM buffer exchange. Any other value selects UDP. | Absent: UDP. Used only when `DEBUG_FUNC_ENABLE_VDEV` is nonzero. | `DEBUG_FUNC_VDEV_SIMSYNC_TYPE MMAP` |

### UDP transport

| Key | Meaning and accepted values | Default and dependency | Example line |
| --- | --- | --- | --- |
| `DEBUG_FUNC_VDEV_TX_IPADDR` | Destination IPv4 address in dotted-decimal form, as accepted by `inet_addr`. | `127.0.0.1`. Used in UDP mode. | `DEBUG_FUNC_VDEV_TX_IPADDR 127.0.0.1` |
| `DEBUG_FUNC_VDEV_TX_PORTNO` | Decimal destination UDP port; use 1 through 65535. | No default; VDEV initialization asserts if it is absent. Used in UDP mode. | `DEBUG_FUNC_VDEV_TX_PORTNO 50000` |
| `DEBUG_FUNC_VDEV_RX_IPADDR` | Local IPv4 address in dotted-decimal form on which the UDP transport creates its receive endpoint. | `127.0.0.1`. Used in UDP mode. | `DEBUG_FUNC_VDEV_RX_IPADDR 127.0.0.1` |
| `DEBUG_FUNC_VDEV_RX_PORTNO` | Decimal local UDP port; use 1 through 65535. | No default; VDEV initialization asserts if it is absent. Used in UDP mode. | `DEBUG_FUNC_VDEV_RX_PORTNO 50001` |
| `DEVICE_CONFIG_COMPLEMENTAL_TX_SENDING` | Nonzero starts a host thread that sends the current transmit packet when no guest-triggered send has occurred for 10 milliseconds of wall time. The spelling `COMPLEMENTAL` is part of the interface. | `0`. If enabled, both reset-area values below must be nonzero. Used in UDP mode. | `DEVICE_CONFIG_COMPLEMENTAL_TX_SENDING 1` |
| `DEVICE_CONFIG_RESET_AREA_OFFSET` | Decimal byte offset of a region cleared after a guest-triggered UDP send. Keep the offset and size within the 1 KiB transmit packet. | `0`. Must be nonzero when complemental sending is enabled. | `DEVICE_CONFIG_RESET_AREA_OFFSET 64` |
| `DEVICE_CONFIG_RESET_AREA_SIZE` | Decimal byte length of the transmit-packet region cleared after a guest-triggered UDP send. | `0`. Must be nonzero when complemental sending is enabled. | `DEVICE_CONFIG_RESET_AREA_SIZE 16` |

### Guest-RAM buffer transport

| Key | Meaning and accepted values | Default and dependency | Example line |
| --- | --- | --- | --- |
| `DEBUG_FUNC_VDEV_MMAP_TX` | Hexadecimal guest RAM address of the transmit synchronization buffer. VDEV writes its header, body, and timestamps there. | No default; initialization asserts if the key is absent or the address does not resolve to RAM. Required when the synchronization type is `MMAP`. | `DEBUG_FUNC_VDEV_MMAP_TX 0x05FFF000` |
| `DEBUG_FUNC_VDEV_MMAP_RX` | Hexadecimal guest RAM address of the receive synchronization buffer written by the companion simulator. | No default; initialization asserts if the key is absent or the address does not resolve to RAM. Required when the synchronization type is `MMAP`. | `DEBUG_FUNC_VDEV_MMAP_RX 0x05FFE000` |
