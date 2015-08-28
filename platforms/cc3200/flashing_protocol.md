# CC3200 ROM communication protocol

All communication is done over one serial port at baud rate 921600 (no
autodetection is performed). In most situations data is transmitted in frames,
each frames must be acknowledged by the remote side (by sending 2 bytes:
"\x00\xCC").

## Frame format

Frames are length-prefixed, with no delimiters inbetween.

Length  | Description
--------|---------------------------------------------------------------
2 bytes | big-endian number, length of payload plus length of this field
1 byte  | checksum, sum of all payload bytes modulo 256
N bytes | payload

First byte of the payload usually is an opcode, rest is the arguments (numbers
are big-endian, if not stated explicitly).

Before sending any frames to a freshly booted board you need to send break
first, then read back one ACK ("\x00\xCC").

## Known opcodes

Opcodes are sorted by numerical value.

 * `0x21` – start file upload.
   * Arguments:
     * 2 bytes: 0
     * 1 byte: `0x30` + block size (0 – 256, 1 – 1024, 2 – 4096, 3 – 16384)
     * 1 byte: number of blocks (in terms of the above block size)
     * N bytes: target filename (usually `/sys/mcuimg.bin`) followed by 2 zero bytes
   * After sending ACK the device will send 4 zero bytes.
 * `0x22` – finish file upload.
   * Arguments:
     * 63 bytes: all 0
     * 256 bytes: all 0x46
     * 1 byte: 0
 * `0x24` – file chunk. Each chunk carries 4096 bytes of the file (not the block
   size specified in `0x21`).
   * Arguments:
     * 4 bytes: offset of the chunk
     * 4096 bytes: data
 * `0x2A` – get file info.
   * Arguments:
     * 4 bytes: name length
     * N bytes: file name
   * Response:
     * 24 bytes: first byte is 1 if the file exists, 4 bytes at offset 4 – file size
 * `0x2B` – get file chunk.
   * Arguments:
     * 4 bytes: offset
     * 4 bytes: number of bytes to read
   * Response:
     * N bytes: content
 * `0x2D` – raw storage write.
   * Arguments:
     * 4 bytes: storage ID – 0
     * 4 bytes: offset
     * 4 bytes: length
     * N bytes: data
 * `0x2E` – file deletion.
   * Arguments:
     * 4 bytes: 0
     * N bytes: zero-terminated filename.
 * `0x2F` – version info.
   * Response is sent in a new frame that you must ACK:
     * 28 bytes: some structure with first 4 bytes containing bootloader version.
 * `0x30` – raw storage block erasing.
   * Arguments:
     * 4 bytes: storage ID – 0
     * 4 bytes: index of the first block to erase (block size can be determined with command `0x31`)
     * 4 bytes: number of blocks to erase
 * `0x31` – get storage info.
   * Arguments:
     * 4 bytes: storage ID – 0
   * Response is sent in a new frame that you must ACK:
     * 2 bytes: block size
     * 2 bytes: number of blocks
 * `0x32` – exec. Sometimes you need to read 2 ACKs instead of one.
 * `0x33` – switch UART pins to another port.
   * Arguments:
     * 4 bytes: magic number `0x0196E6AB`

