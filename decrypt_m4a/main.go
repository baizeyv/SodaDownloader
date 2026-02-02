package main

import (
	"fmt"
	"io/ioutil"
	"log"
	"os"
)

// TIP <p>To run your code, right-click the code and select <b>Run</b>.</p> <p>Alternatively, click
// the <icon src="AllIcons.Actions.Execute"/> icon in the gutter and select the <b>Run</b> menu item from here.</p>
func main() {
	fmt.Fprintf(os.Stdout, "[EXE]!", os.Args[0])

	for i, arg := range os.Args {
		fmt.Fprintf(os.Stderr, "Arg[%d]: %q\n", i, arg)
	}

	fmt.Fprintf(os.Stdout, "%d\n", len(os.Args))
	if len(os.Args) != 4 {
		fmt.Fprintf(os.Stderr, "Usage: %s <inputPath> <spade_a> <outputPath>\n", os.Args[0])
		os.Exit(1)
	}

	// 文件路径和播放凭证
	filePath := os.Args[1]
	playAuth := os.Args[2]
	outputPath := os.Args[3]

	// 读取文件
	fileData, err := ioutil.ReadFile(filePath)
	if err != nil {
		log.Fatalf("读取文件失败: %v", err)
	}

	// 调用解密方法
	decryptedData, err := DecryptAudio(fileData, playAuth)
	if err != nil {
		log.Fatalf("解密失败: %v", err)
	}

	// 保存解密后的文件
	err = ioutil.WriteFile(outputPath, decryptedData, 0644)
	if err != nil {
		log.Fatalf("保存文件失败: %v", err)
	}

	fmt.Printf("解密成功！文件已保存到: %s\n", outputPath)
	fmt.Printf("原始大小: %d bytes, 解密后大小: %d bytes\n",
		len(fileData), len(decryptedData))
}
