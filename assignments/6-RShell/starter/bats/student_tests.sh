#!/usr/bin/env bats

@test "Example: check ls runs without errors" {
    run ./dsh <<EOF
ls
EOF
    [ "$status" -eq 0 ]
}

@test "Test: Echo and awk together in pipe" {
    run "./dsh" <<EOF
echo Alice 25 Engineer | awk {print\$1}
EOF
    stripped_output=$(echo "$output" | tr -d '[:space:]')
    expected_output="localmodedsh4>Alice25Engineerdsh4>cmdloopreturned0"
    echo "Captured stdout:"
    echo "Output: $output"
    echo "Exit Status: $status"
    echo "${stripped_output} -> ${expected_output}"
    [ "$stripped_output" = "$expected_output" ]
    [ "$status" -eq 0 ]
}

@test "Test: Cat non existent file" {
    run "./dsh" <<EOF
cat non_existent_file | grep anything
EOF
    stripped_output=$(echo "$output" | tr -d '[:space:]')
    expected_output="localmodedsh4>cat:non_existent_file:Nosuchfileordirectorydsh4>cmdloopreturned0"
    echo "Captured stdout:"
    echo "Output: $output"
    echo "Exit Status: $status"
    echo "${stripped_output} -> ${expected_output}"
    [ "$stripped_output" = "$expected_output" ]
    [ "$status" -eq 0 ]
}

@test "Test: Multiple pipes put together pt1" {
    run "./dsh" <<EOF
ls | grep .c | wc -l
EOF
    stripped_output=$(echo "$output" | tr -d '[:space:]')
    expected_output="localmodedsh4>4dsh4>cmdloopreturned0"
    echo "Captured stdout:"
    echo "Output: $output"
    echo "Exit Status: $status"
    echo "${stripped_output} -> ${expected_output}"
    [ "$stripped_output" = "$expected_output" ]
    [ "$status" -eq 0 ]
}

@test "Test: Multiple pipes put together pt2" {
    run "./dsh" <<EOF
echo hello | grep hello | wc -w
EOF
    stripped_output=$(echo "$output" | tr -d '[:space:]')
    expected_output="localmodedsh4>1dsh4>cmdloopreturned0"
    echo "Captured stdout:"
    echo "Output: $output"
    echo "Exit Status: $status"
    echo "${stripped_output} -> ${expected_output}"
    [ "$stripped_output" = "$expected_output" ]
    [ "$status" -eq 0 ]
}

@test "Test: Tail command functions with ls" {
    run "./dsh" <<EOF
ls | tail -n 1
EOF
    stripped_output=$(echo "$output" | tr -d '[:space:]')
    [[ "$stripped_output" =~ "localmodedsh4>" ]] && [[ "$stripped_output" =~ "dsh4>cmdloopreturned0" ]]
    [ "$status" -eq 0 ]
}

@test "Test: Grep non existent file" {
    run "./dsh" <<EOF
ls | grep non_existent_file
EOF
    stripped_output=$(echo "$output" | tr -d '[:space:]')
    expected_output="localmodedsh4>dsh4>cmdloopreturned0"
    echo "Captured stdout:"
    echo "Output: $output"
    echo "Exit Status: $status"
    echo "${stripped_output} -> ${expected_output}"
    [ "$stripped_output" = "$expected_output" ]
    [ "$status" -eq 0 ]
}

@test "Test: Pipe with empty input" {
    run "./dsh" <<EOF
echo | grep hello
EOF
    stripped_output=$(echo "$output" | tr -d '[:space:]')
    expected_output="localmodedsh4>dsh4>cmdloopreturned0"
    echo "Captured stdout:"
    echo "Output: $output"
    echo "Exit Status: $status"
    echo "${stripped_output} -> ${expected_output}"
    [ "$stripped_output" = "$expected_output" ]
    [ "$status" -eq 0 ]
}

@test "Test: Pipe with no output" {
    run "./dsh" <<EOF
echo "hello" | grep non_existent_word
EOF
    stripped_output=$(echo "$output" | tr -d '[:space:]')
    expected_output="localmodedsh4>dsh4>cmdloopreturned0"
    echo "Captured stdout:"
    echo "Output: $output"
    echo "Exit Status: $status"
    echo "${stripped_output} -> ${expected_output}"
    [ "$stripped_output" = "$expected_output" ]
    [ "$status" -eq 0 ]
}

@test "Test: Change directory" {
    run "./dsh" <<EOF
cd ..
pwd
EOF
    [ "$status" -eq 0 ]
}

@test "Test: Change directory - no args" {
    run "./dsh" <<EOF
cd
pwd
EOF
    [[ "$output" =~ "missing argument" ]]
    [ "$status" -eq 0 ]
}

@test "Test: Exit command" {
    run "./dsh" <<EOF
exit
EOF
    [ "$status" -eq 0 ]
}

@test "Test: Grep finds sentence in test file" {
    echo "is there stuff within this file? who knows" > testfile.txt
    
    run "./dsh" <<EOF
grep stuff testfile.txt
EOF
    stripped_output=$(echo "$output" | tr -d '[:space:]')
    expected_output="localmodedsh4>istherestuffwithinthisfile?whoknowsdsh4>cmdloopreturned0"
    echo "Captured stdout:"
    echo "Output: $output"
    echo "Exit Status: $status"
    echo "${stripped_output} -> ${expected_output}"
    [ "$stripped_output" = "$expected_output" ]
    [ "$status" -eq 0 ]
    
    rm -f testfile.txt
}

@test "Test: Empty Command gives no commands message" {
    run "./dsh" <<EOF

EOF
    [[ "$output" =~ "warning: no commands provided" ]]
    [ "$status" -eq 0 ]
}

@test "Test: Invalid command with special characters" {
    run "./dsh" <<EOF
ls @#$%^&*()
EOF
    [[ "$output" =~ "No such file or directory" ]]
    [ "$status" -eq 0 ]
}

@test "Test: Pipe to invalid command" {
    run "./dsh" <<EOF
echo hello | invalid_command
EOF
    [[ "$output" =~ "No such file or directory" ]]
    [ "$status" -eq 0 ]
}

@test "Test: Pipe with empty string input" {
    run "./dsh" <<EOF
echo "" | grep hello
EOF
    stripped_output=$(echo "$output" | tr -d '[:space:]')
    expected_output="localmodedsh4>dsh4>cmdloopreturned0"
    echo "Captured stdout:"
    echo "Output: $output"
    echo "Exit Status: $status"
    echo "${stripped_output} -> ${expected_output}"
    [ "$stripped_output" = "$expected_output" ]
    [ "$status" -eq 0 ]
}

@test "Test: Command with multiple arguments (proper space handling)" {
    run "./dsh" <<EOF
echo hello world
EOF
    [[ "$output" =~ "hello world" ]]
    [ "$status" -eq 0 ]
}

@test "Test: Cant go past 8 commands in a pipeline" {
    run "./dsh" <<EOF
echo command1 | echo command2 | echo command3 | echo command4 | echo command5 | echo command6 | echo command7 | echo command8 | echo command9
EOF
    [[ "$output" =~ "error: piping limited to 8 commands" ]]
    [ "$status" -eq 0 ]
}
