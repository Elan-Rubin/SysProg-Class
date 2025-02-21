#!/usr/bin/env bats

@test "cd with no arguments does nothing" {
    run ./dsh <<EOF
pwd
cd
pwd
EOF
    
    # Get the lines from output
    pwd1=$(echo "$output" | grep -m1 "/")
    pwd2=$(echo "$output" | grep -m2 "/" | tail -n1)
    
    # Check that PWD didn't change
    [ "$pwd1" = "$pwd2" ]
}

@test "cd with argument changes directory" {
    current=$(pwd)
    
    cd /tmp
    mkdir -p dsh-test
    
    run ./dsh <<EOF
cd dsh-test
pwd
EOF

    [ "$status" -eq 0 ]
    [[ "$output" =~ "/tmp/dsh-test" ]]
    
    # Cleanup
    rm -rf /tmp/dsh-test
}

@test "external command executes correctly" {
    run ./dsh <<EOF
echo "test message"
EOF

    [ "$status" -eq 0 ]
    [[ "$output" =~ "test message" ]]
}

@test "command with arguments works" {
    run ./dsh <<EOF
ls -la
EOF

    [ "$status" -eq 0 ]
}

@test "quoted strings are preserved" {
    run ./dsh <<EOF
echo "  hello    world  "
EOF

    [[ "$output" =~ "  hello    world  " ]]
}

@test "rc command shows last return code" {
    run ./dsh <<EOF
not_a_real_command
rc
EOF

    [[ "$output" =~ "Command not found in PATH" ]]
    [[ "$output" =~ "2" ]]  # ENOENT error code
}

@test "handles permission denied error" {
    # Create a file without execute permissions
    touch /tmp/not_executable
    chmod 644 /tmp/not_executable
    
    run ./dsh <<EOF
/tmp/not_executable
rc
EOF

    [[ "$output" =~ "Permission denied" ]]
    [[ "$output" =~ "13" ]]  # EACCES error code
    
    # Cleanup
    rm /tmp/not_executable
}

@test "empty command gives warning" {
    run ./dsh <<EOF

EOF

    [[ "$output" =~ "warning: no commands provided" ]]
}

@test "exit command works" {
    run ./dsh <<EOF
exit
EOF

    [[ "$output" =~ "cmd loop returned 0" ]]
    [ "$status" -eq 0 ]
}