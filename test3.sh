#!/bin/bash
# test3.sh

echo "===== Test 3: Wildcards & Conditionals ====="

# 1. Wildcard expansion
echo "Listing all .c files:"
ls *.c
echo "Listing all .txt files:"
ls *.txt

# 2. Conditionals
echo "Testing 'falseCommand' with 'and':"
falseCommand
and echo "You should NOT see this message, because 'falseCommand' should fail."

echo "Testing 'falseCommand' with 'or':"
falseCommand
or echo "You SHOULD see this message, because 'falseCommand' failed."

echo "Testing 'ls' with 'and':"
ls
and echo "'ls' succeeded, so we see this message."

echo "Done with test3.sh"