package esp32

import (
	"bytes"
	"crypto/aes"
	"crypto/cipher"

	"github.com/cesanta/errors"
)

// ESP32EncryptImageData implements the algorithm described here:
// https://esp-idf.readthedocs.io/en/latest/security/flash-encryption.html#flash-encryption-algorithm

const (
	esp32EncryptionBlockSize        = aes.BlockSize
	esp32EncryptionKeyLength        = 32
	esp32EncryptionKeyTweakInterval = 32
)

func reverse(data []byte) {
	l := len(data)
	for i := 0; i < l/2; i++ {
		data[i], data[l-i-1] = data[l-i-1], data[i]
	}
}

func ESP32EncryptImageData(inData, key []byte, flashAddress, flashCryptConf uint32) ([]byte, error) {
	if len(key) != esp32EncryptionKeyLength {
		return nil, errors.Errorf("encryption key must be %d bytes, got %d", esp32EncryptionKeyLength, len(key))
	}
	if flashAddress%esp32EncryptionBlockSize != 0 {
		return nil, errors.Errorf("flash address must be divisible by %d", esp32EncryptionBlockSize)
	}
	inBuf := bytes.NewBuffer(inData)
	outData := make([]byte, 0, len(inData))
	blockData := make([]byte, esp32EncryptionBlockSize)
	var cipher cipher.Block
	for {
		n, _ := inBuf.Read(blockData)
		if n == 0 {
			break
		}
		for n < esp32EncryptionBlockSize {
			/* Pad with zeroes. */
			blockData[n] = 0
			n++
		}
		if cipher == nil || flashAddress%esp32EncryptionKeyTweakInterval == 0 {
			blockKey := make([]byte, esp32EncryptionKeyLength)
			copy(blockKey, key)
			esp32EncryptionTweakKey(blockKey, flashAddress, flashCryptConf)
			cipher, _ = aes.NewCipher(blockKey)
		}
		reverse(blockData)
		cipher.Decrypt(blockData, blockData)
		reverse(blockData)
		outData = append(outData, blockData...)
		flashAddress += esp32EncryptionBlockSize
	}

	return outData, nil
}

func esp32EncryptionTweakKeyBits(key []byte, flashAddress, keyBitIdxFrom, keyBitIdxTo uint32) {
	for keyBitIdx := keyBitIdxFrom; keyBitIdx < keyBitIdxTo; keyBitIdx++ {
		addrBitIdx := esp32EncryptionKeyBitTweakPattern[keyBitIdx]
		addrMask := uint32(1) << addrBitIdx
		if flashAddress&addrMask != 0 {
			keyByteIdx := keyBitIdx / 8
			keyByteMask := byte(1) << (7 - (keyBitIdx % 8))
			key[keyByteIdx] ^= keyByteMask
		}
	}
}

func esp32EncryptionTweakKey(key []byte, flashAddress, flashCryptConf uint32) {
	if flashCryptConf&1 != 0 {
		esp32EncryptionTweakKeyBits(key, flashAddress, 0, 67)
	}
	if flashCryptConf&2 != 0 {
		esp32EncryptionTweakKeyBits(key, flashAddress, 67, 132)
	}
	if flashCryptConf&4 != 0 {
		esp32EncryptionTweakKeyBits(key, flashAddress, 132, 195)
	}
	if flashCryptConf&8 != 0 {
		esp32EncryptionTweakKeyBits(key, flashAddress, 195, 256)
	}
}

var esp32EncryptionKeyBitTweakPattern = []uint{
	23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5,
	23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5,
	23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5,
	14, 13, 12, 11, 10, 9, 8, 7, 6, 5,
	23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5,
	23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5,
	23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5,
	12, 11, 10, 9, 8, 7, 6, 5,
	23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5,
	23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5,
	23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5,
	10, 9, 8, 7, 6, 5,
	23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5,
	23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5,
	23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5,
	8, 7, 6, 5,
}
