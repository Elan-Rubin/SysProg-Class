#!/bin/bash

RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m' # No Color

echo -e "${GREEN}Starting Remote Shell Tests${NC}"

# Helper function for tests
function run_test() {
    local test_name=$1
    local command=$2
    local expected=$3
    
    echo -e "\n${GREEN}Test: $test_name${NC}"
    echo "Command: $command"
    
    # Run the command and capture output
    output=$(eval "$command")
    
    # Check result
    if [[ "$output" == *"$expected"* ]]; then
        echo -e "${GREEN}PASS: Output contains expected text${NC}"
    else
        echo -e "${RED}FAIL: Output does not contain expected text${NC}"
        echo "Expected to contain: $expected"
        echo "Actual output: $output"
    fi
}

# 1. First compile the code
echo "Compiling..."
make clean && make

if [ $? -ne 0 ]; then
    echo -e "${RED}Compilation failed. Exiting tests.${NC}"
    exit 1
fi

# 2. Start server in background for testing
PORT=4321
echo "Starting server on port $PORT..."
./dsh -s -p $PORT &
SERVER_PID=$!

# Give server time to start
sleep 1

# 3. Simple command test
run_test "Simple command" \
    "echo 'echo hello' | ./dsh -c -p $PORT" \
    "hello"

# 4. Multiple commands test
run_test "Multiple commands" \
    "echo -e 'pwd\necho test' | ./dsh -c -p $PORT" \
    "test"

# 5. Pipeline test
run_test "Pipeline" \
    "echo 'ls -la | grep dsh' | ./dsh -c -p $PORT" \
    "dsh"

# 6. Built-in command test (CD)
run_test "CD command" \
    "echo -e 'cd /tmp\npwd' | ./dsh -c -p $PORT" \
    "/tmp"

# 7. Built-in command test (DRAGON)
run_test "Dragon command" \
    "echo 'dragon' | ./dsh -c -p $PORT" \
    "(_______________)"

# 8. Error handling test
run_test "Invalid command" \
    "echo 'nonexistentcommand' | ./dsh -c -p $PORT" \
    "Error"

# 9. Stop server test
echo -e "\n${GREEN}Test: Stop server${NC}"
echo "Command: stop-server"
echo "stop-server" | ./dsh -c -p $PORT

# Check if server process is terminated
sleep 2
if ps -p $SERVER_PID > /dev/null; then
    echo -e "${RED}FAIL: Server is still running after stop-server command${NC}"
    kill $SERVER_PID
else
    echo -e "${GREEN}PASS: Server process terminated successfully${NC}"
fi

# Start server again to test client exit
echo "Starting server again..."
./dsh -s -p $PORT &
SERVER_PID=$!
sleep 1

# 10. Client exit test
run_test "Client exit" \
    "echo 'exit' | ./dsh -c -p $PORT" \
    "Goodbye"

# 11. Test stdin/stdout redirection
run_test "Standard I/O redirection" \
    "echo 'cat < /etc/hosts' | ./dsh -c -p $PORT" \
    "localhost"

# 12. Test long output
run_test "Long output handling" \
    "echo 'cat /etc/passwd | head -20' | ./dsh -c -p $PORT" \
    "root"

# Cleanup
kill $SERVER_PID 2>/dev/null
wait $SERVER_PID 2>/dev/null

echo -e "\n${GREEN}All tests completed.${NC}"