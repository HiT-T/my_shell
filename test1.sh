echo "===== Test 1: Basic External & Built-ins ====="

echo "Running 'echo' and 'pwd':"
echo "Hello from test1.sh"
pwd

echo "Attempting to 'cd subdir'..."
cd subdir
pwd

echo "Which command for 'ls':"
which ls

echo "The next line won't run if exit works properly."
exit
echo "If you see this line, 'exit' didn't stop the script."