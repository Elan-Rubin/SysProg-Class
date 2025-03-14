#!/usr/bin/env bats

@test "Example: check ls runs without errors" {
    run ./dsh <<EOF
ls
EOF
    [ "$status" -eq 0 ]
}

@test "Test: Echo and awk together in pipe" {
    run "./dsh" <<EOF
echo Alice 25 Engineer | awk {print$1}
EOF
    stripped_output=$(echo "$output" | tr -d '[:space:]')
    expected_output="Alicelocalmodedsh4>dsh4>cmdloopreturned0"
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
    expected_output="cat:non_existent_file:Nosuchfileordirectorylocalmodedsh4>dsh4>cmdloopreturned0"
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
    expected_output="5localmodedsh4>dsh4>cmdloopreturned0"
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
    expected_output="1localmodedsh4>dsh4>cmdloopreturned0"
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
    expected_output="testfile.txtlocalmodedsh4>dsh4>cmdloopreturned0"
    echo "Captured stdout:"
    echo "Output: $output"
    echo "Exit Status: $status"
    echo "${stripped_output} -> ${expected_output}"
    [ "$stripped_output" = "$expected_output" ]
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
ls
EOF
    stripped_output=$(echo "$output" | tr -d '[:space:]')
    expected_output="questions.mdreadme.mdstarterlocalmodedsh4>dsh4>dsh4>cmdloopreturned0"
    echo "Captured stdout:"
    echo "Output: $output"
    echo "Exit Status: $status"
    echo "${stripped_output} -> ${expected_output}"
    [ "$stripped_output" = "$expected_output" ]
}

@test "Test: Change directory - no args" {
    run "./dsh" <<EOF
cd
ls
EOF
    stripped_output=$(echo "$output" | tr -d '[:space:]')
    expected_output="batsdragon.cdshdsh_cli.cdshlib.cdshlib.hmakefilersh_cli.crsh_server.crshlib.htestfile.txtlocalmodedsh4>dsh4>dsh4>cmdloopreturned0"
    echo "Captured stdout:"
    echo "Output: $output"
    echo "Exit Status: $status"
    echo "${stripped_output} -> ${expected_output}"
    [ "$stripped_output" = "$expected_output" ]
}

@test "Test: Exit command" {
    run "./dsh" <<EOF
exit
EOF
    [ "$status" -eq 0 ]
}

@test "Test: Grep finds sentence in test file" {
    run "./dsh" <<EOF
grep stuff testfile.txt
EOF
    stripped_output=$(echo "$output" | tr -d '[:space:]')
    expected_output="istherestuffwithinthisfile?whoknowslocalmodedsh4>dsh4>cmdloopreturned0"
    echo "Captured stdout:"
    echo "Output: $output"
    echo "Exit Status: $status"
    echo "${stripped_output} -> ${expected_output}"
    [ "$stripped_output" = "$expected_output" ]
}

@test "Test: Empty Command gives no commands message" {
    run "./dsh" <<EOF

EOF
    stripped_output=$(echo "$output" | tr -d '[:space:]')
    expected_output="localmodedsh4>warning:nocommandsprovideddsh4>cmdloopreturned0"
    echo "Captured stdout:"
    echo "Output: $output"
    echo "Exit Status: $status"
    echo "${stripped_output} -> ${expected_output}"
    [ "$stripped_output" = "$expected_output" ]
}

@test "Test: Invalid command with special characters" {
    run "./dsh" <<EOF
ls @#$%^&*()
EOF
    stripped_output=$(echo "$output" | tr -d '[:space:]')
    expected_output="ls:cannotaccess'@#$%^&*()':Nosuchfileordirectorylocalmodedsh4>dsh4>cmdloopreturned0"
    echo "Captured stdout:"
    echo "Output: $output"
    echo "Exit Status: $status"
    echo "${stripped_output} -> ${expected_output}"
    [ "$stripped_output" = "$expected_output" ]
}

@test "Test: Pipe to invalid command" {
    run "./dsh" <<EOF
echo hello | invalid_command
EOF
    stripped_output=$(echo "$output" | tr -d '[:space:]')
    expected_output="execvp:Nosuchfileordirectorylocalmodedsh4>dsh4>cmdloopreturned0"
    echo "Captured stdout:"
    echo "Output: $output"
    echo "Exit Status: $status"
    echo "${stripped_output} -> ${expected_output}"
    [ "$stripped_output" = "$expected_output" ]
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
    stripped_output=$(echo "$output" | tr -d '[:space:]')
    expected_output="helloworldlocalmodedsh4>dsh4>cmdloopreturned0"
    echo "Captured stdout:"
    echo "Output: $output"
    echo "Exit Status: $status"
    echo "${stripped_output} -> ${expected_output}"
    [ "$stripped_output" = "$expected_output" ]
    [ "$status" -eq 0 ]
}

@test "Test: Cant go past 8 commands in a pipeline" {
    run "./dsh" <<EOF
echo command1 | echo command2 | echo command3 | echo command4 | echo command5 | echo command6 | echo command7 | echo command8 | echo command9
EOF
    stripped_output=$(echo "$output" | tr -d '[:space:]')
    expected_output="localmodedsh4>error:pipinglimitedto8commandsdsh4>cmdloopreturned0"
    echo "Captured stdout:"
    echo "Output: $output"
    echo "Exit Status: $status"
    echo "${stripped_output} -> ${expected_output}"
    [ "$stripped_output" = "$expected_output" ]
    [ "$status" -eq 0 ]
}
