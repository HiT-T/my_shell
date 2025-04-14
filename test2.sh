#!/bin/bash
# test2.sh

echo "===== Test 2: Redirection & Pipes ====="

# 1. Redirecting input
echo "Reading from input.txt and piping to 'grep':"
cat < input.txt | grep "pattern"

# 2. Redirecting output
echo "Listing files to out.txt..."
ls > out.txt

echo "Contents of out.txt:"
cat out.txt

# 3. Piping a built-in to an external command
echo "Piping 'pwd' into 'wc -c':"
pwd | wc -c