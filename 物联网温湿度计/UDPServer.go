package main

import (
	"bufio"
	"fmt"
	"log"
	"net"
	"os"
	"strings"
)

// 配置本地 IP 地址（可修改）
const localIP = "192.168.0.159"
const localPort = 8080

func main() {
	// 在 goroutine 中启动 UDP 服务器
	go startUDPServer()

	// 主线程处理用户输入
	fmt.Println("UDP 服务器已启动，监听端口 8080...")
	fmt.Printf("本地 IP: %s, 端口: %d\n", localIP, localPort)
	fmt.Println("输入格式: IP地址:端口 (例如: 192.168.0.100:8080)")
	fmt.Println("输入 'exit' 退出程序")
	fmt.Println("---")

	scanner := bufio.NewScanner(os.Stdin)
	for {
		// fmt.Print("请输入目标 IP:端口 > ")
		if !scanner.Scan() {
			break
		}

		input := strings.TrimSpace(scanner.Text())
		if input == "exit" {
			fmt.Println("程序退出")
			break
		}

		// 解析输入的 IP 和端口
		parts := strings.Split(input, ":")
		if len(parts) != 2 {
			fmt.Println("格式错误！请输入 IP:端口 (例如: 192.168.0.100:8080)")
			continue
		}

		targetIP := parts[0]
		targetPort := parts[1]

		// 发送 UDP 消息
		err := sendUDPMessage(targetIP, targetPort)
		if err != nil {
			log.Printf("发送失败: %v\n", err)
		} else {
			fmt.Printf("已向 %s:%s 发送消息: hello\n", targetIP, targetPort)
		}
	}
}

// 启动 UDP 服务器监听
func startUDPServer() {
	// 监听 UDP 8080 端口
	addr, err := net.ResolveUDPAddr("udp", fmt.Sprintf(":%d", localPort))
	if err != nil {
		log.Fatalf("解析地址失败: %v", err)
	}

	// 创建 UDP 连接
	conn, err := net.ListenUDP("udp", addr)
	if err != nil {
		log.Fatalf("监听 UDP 失败: %v", err)
	}
	defer conn.Close()

	fmt.Println("等待接收数据...")

	// 缓冲区用于接收数据
	buffer := make([]byte, 4096)

	for {
		// 接收 UDP 数据包
		n, clientAddr, err := conn.ReadFromUDP(buffer)
		if err != nil {
			log.Printf("读取数据失败: %v", err)
			continue
		}

		// 显示接收到的数据
		data := string(buffer[:n])
		fmt.Printf("\n[收到消息] 来自 %s: %s", clientAddr.String(), data)
		// fmt.Printf("数据长度: %d 字节\n", n)
		// fmt.Print("请输入目标 IP:端口 > ")
	}
}

// 发送 UDP 消息
func sendUDPMessage(targetIP, targetPort string) error {
	// 解析目标地址
	targetAddr, err := net.ResolveUDPAddr("udp", fmt.Sprintf("%s:%s", targetIP, targetPort))
	if err != nil {
		return fmt.Errorf("解析目标地址失败: %v", err)
	}

	// 解析本地地址
	localAddr, err := net.ResolveUDPAddr("udp", fmt.Sprintf("%s:%d", localIP, localPort))
	if err != nil {
		return fmt.Errorf("解析本地地址失败: %v", err)
	}

	// 创建 UDP 连接
	conn, err := net.DialUDP("udp", localAddr, targetAddr)
	if err != nil {
		return fmt.Errorf("创建 UDP 连接失败: %v", err)
	}
	defer conn.Close()

	// 发送消息
	message := "hello"
	_, err = conn.Write([]byte(message))
	if err != nil {
		return fmt.Errorf("发送数据失败: %v", err)
	}

	return nil
}