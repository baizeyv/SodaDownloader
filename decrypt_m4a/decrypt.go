package main

import (
	"bytes"
	"crypto/aes"
	"crypto/cipher"
	"encoding/base64"
	"encoding/binary"
	"encoding/hex"
	"errors"
)

// DecryptAudio 核心解密函数
func DecryptAudio(fileData []byte, playAuth string) ([]byte, error) {
	hexKey, err := extractKey(playAuth)
	if err != nil {
		return nil, err
	}
	keyBytes, err := hex.DecodeString(hexKey)
	if err != nil {
		return nil, err
	}

	moov, err := findBox(fileData, "moov", 0, len(fileData))
	if err != nil {
		return nil, errors.New("moov box not found")
	}

	stbl, err := findBox(fileData, "stbl", moov.offset, moov.offset+moov.size)
	if err != nil {
		trak, _ := findBox(fileData, "trak", moov.offset+8, moov.offset+moov.size)
		if trak != nil {
			mdia, _ := findBox(fileData, "mdia", trak.offset+8, trak.offset+trak.size)
			if mdia != nil {
				minf, _ := findBox(fileData, "minf", mdia.offset+8, mdia.offset+mdia.size)
				if minf != nil {
					stbl, _ = findBox(fileData, "stbl", minf.offset+8, minf.offset+minf.size)
				}
			}
		}
	}
	if stbl == nil {
		return nil, errors.New("stbl box not found")
	}

	stsz, err := findBox(fileData, "stsz", stbl.offset+8, stbl.offset+stbl.size)
	if err != nil {
		return nil, errors.New("stsz box not found")
	}
	sampleSizes := parseStsz(stsz.data)

	senc, err := findBox(fileData, "senc", moov.offset+8, moov.offset+moov.size)
	if err != nil {
		senc, err = findBox(fileData, "senc", stbl.offset+8, stbl.offset+stbl.size)
	}
	if err != nil {
		return nil, errors.New("senc box not found")
	}
	ivs := parseSenc(senc.data)

	mdat, err := findBox(fileData, "mdat", 0, len(fileData))
	if err != nil {
		return nil, errors.New("mdat box not found")
	}

	block, err := aes.NewCipher(keyBytes)
	if err != nil {
		return nil, err
	}

	decryptedData := make([]byte, len(fileData))
	copy(decryptedData, fileData)

	readPtr := mdat.offset + 8
	decryptedMdat := make([]byte, 0, mdat.size-8)

	for i := 0; i < len(sampleSizes); i++ {
		size := int(sampleSizes[i])
		if readPtr+size > len(decryptedData) {
			break
		}
		chunk := decryptedData[readPtr : readPtr+size]

		if i < len(ivs) {
			iv := ivs[i]
			if len(iv) < 16 {
				padded := make([]byte, 16)
				copy(padded, iv)
				iv = padded
			}
			stream := cipher.NewCTR(block, iv)
			dst := make([]byte, size)
			stream.XORKeyStream(dst, chunk)
			decryptedMdat = append(decryptedMdat, dst...)
		} else {
			decryptedMdat = append(decryptedMdat, chunk...)
		}
		readPtr += size
	}

	if len(decryptedMdat) == int(mdat.size)-8 {
		copy(decryptedData[mdat.offset+8:], decryptedMdat)
	} else {
		return nil, errors.New("decrypted size mismatch")
	}

	stsd, err := findBox(fileData, "stsd", stbl.offset+8, stbl.offset+stbl.size)
	if err == nil {
		stsdOffset := stsd.offset
		stsdData := decryptedData[stsdOffset : stsdOffset+stsd.size]
		if idx := bytes.Index(stsdData, []byte("enca")); idx != -1 {
			copy(stsdData[idx:], []byte("mp4a"))
			copy(decryptedData[stsdOffset:], stsdData)
		}
	}

	return decryptedData, nil
}

type mp4Box struct {
	offset int
	size   int
	data   []byte
}

func findBox(data []byte, boxType string, start, end int) (*mp4Box, error) {
	if end > len(data) {
		end = len(data)
	}
	pos := start
	target := []byte(boxType)
	for pos+8 <= end {
		size := int(binary.BigEndian.Uint32(data[pos : pos+4]))
		if size < 8 {
			break
		}
		if bytes.Equal(data[pos+4:pos+8], target) {
			return &mp4Box{offset: pos, size: size, data: data[pos+8 : pos+size]}, nil
		}
		pos += size
	}
	return nil, errors.New("box not found")
}

func parseStsz(data []byte) []uint32 {
	if len(data) < 12 {
		return nil
	}
	sampleSizeFixed := binary.BigEndian.Uint32(data[4:8])
	sampleCount := int(binary.BigEndian.Uint32(data[8:12]))
	sizes := make([]uint32, sampleCount)
	if sampleSizeFixed != 0 {
		for i := 0; i < sampleCount; i++ {
			sizes[i] = sampleSizeFixed
		}
	} else {
		for i := 0; i < sampleCount; i++ {
			if 12+i*4+4 <= len(data) {
				sizes[i] = binary.BigEndian.Uint32(data[12+i*4 : 12+i*4+4])
			}
		}
	}
	return sizes
}

func parseSenc(data []byte) [][]byte {
	if len(data) < 8 {
		return nil
	}
	flags := binary.BigEndian.Uint32(data[0:4]) & 0x00FFFFFF
	sampleCount := int(binary.BigEndian.Uint32(data[4:8]))
	ivs := make([][]byte, 0, sampleCount)
	ptr := 8
	hasSubsamples := (flags & 0x02) != 0
	for i := 0; i < sampleCount; i++ {
		if ptr+8 > len(data) {
			break
		}
		ivs = append(ivs, data[ptr:ptr+8])
		ptr += 8
		if hasSubsamples {
			if ptr+2 > len(data) {
				break
			}
			subCount := int(binary.BigEndian.Uint16(data[ptr : ptr+2]))
			ptr += 2 + (subCount * 6)
		}
	}
	return ivs
}

func bitcount(n int) int {
	u := uint32(n)
	u = u & 0xFFFFFFFF
	u = u - ((u >> 1) & 0x55555555)
	u = (u & 0x33333333) + ((u >> 2) & 0x33333333)
	return int((((u + (u >> 4)) & 0xF0F0F0F) * 0x1010101) >> 24)
}

func decodeBase36(c byte) int {
	if c >= '0' && c <= '9' {
		return int(c - '0')
	}
	if c >= 'a' && c <= 'z' {
		return int(c - 'a' + 10)
	}
	return 0xFF
}

func decryptSpadeInner(keyBytes []byte) []byte {
	result := make([]byte, len(keyBytes))
	buff := append([]byte{0xFA, 0x55}, keyBytes...)
	for i := 0; i < len(result); i++ {
		v := int(keyBytes[i]^buff[i]) - bitcount(i) - 21
		for v < 0 {
			v += 255
		}
		result[i] = byte(v)
	}
	return result
}

func extractKey(playAuth string) (string, error) {
	binaryStr, err := base64.StdEncoding.DecodeString(playAuth)
	if err != nil {
		return "", err
	}
	bytesData := []byte(binaryStr)
	if len(bytesData) < 3 {
		return "", errors.New("auth data too short")
	}
	paddingLen := int((bytesData[0] ^ bytesData[1] ^ bytesData[2]) - 48)
	if len(bytesData) < paddingLen+2 {
		return "", errors.New("invalid padding length")
	}
	innerInput := bytesData[1 : len(bytesData)-paddingLen]
	tmpBuff := decryptSpadeInner(innerInput)
	if len(tmpBuff) == 0 {
		return "", errors.New("decryption failed")
	}
	skipBytes := decodeBase36(tmpBuff[0])
	endIndex := 1 + (len(bytesData) - paddingLen - 2) - skipBytes
	if endIndex > len(tmpBuff) || endIndex < 1 {
		return "", errors.New("index out of bounds")
	}
	return string(tmpBuff[1:endIndex]), nil
}
