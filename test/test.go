package main

import (
	"fmt"
	"net"
	"sync"
	"time"
)

const (
	serverAddr   = "localhost:6667"
	concurrency  = 4000
	testDuration = 5 * time.Second
)

func launchClient(id int, wg *sync.WaitGroup, errors chan<- error) {
	defer wg.Done()

	conn, err := net.Dial("tcp", serverAddr)
	if err != nil {
		errors <- err
		return
	}
	defer conn.Close()

	handshake := fmt.Sprintf("NICK gobot%d\nUSER gobot%d 0 * :stress-test\n", id, id)
	_, err = conn.Write([]byte(handshake))
	if err != nil {
		errors <- err
		return
	}

	ticker := time.NewTicker(1 * time.Second)
	defer ticker.Stop()
	timeout := time.After(testDuration)

	for {
		select {
		case <-ticker.C:
			msg := fmt.Sprintf("PRIVMSG #stress_channel :Load burst from client %d\n", id)
			_, err := conn.Write([]byte(msg))
			if err != nil {
				errors <- err
				return
			}
		case <-timeout:
			conn.Write([]byte("QUIT :Test finished\n"))
			return
		}
	}
}

func main() {
	var wg sync.WaitGroup
	errChan := make(chan error, concurrency*2)

	fmt.Printf("🔥 Launching Go stress test: %d goroutines targeting %s...\n", concurrency, serverAddr)
	startTime := time.Now()

	for i := 1; i <= concurrency; i++ {
		wg.Add(1)
		go launchClient(i, &wg, errChan)
	}

	wg.Wait()
	close(errChan)

	totalErrors := 0
	for range errChan {
		totalErrors++
	}

	fmt.Println("\n📊 --- STRESS TEST REPORT ---")
	fmt.Printf("⏱️  Test duration: %v\n", time.Since(startTime))
	fmt.Printf("👥 Simulated clients: %d\n", concurrency)
	fmt.Printf("❌ Failed/Dropped connections: %d\n", totalErrors)

	if totalErrors == 0 {
		fmt.Println("🟢 SUCCESS! Your C++ server handled the high concurrency flawlessly.")
	} else {
		fmt.Println("🔴 WARNING! Socket drops detected. Review your poll() loop FDs.")
	}
}
