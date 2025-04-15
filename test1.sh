echo "===== Test 1: Basic External & Built-ins ====="

echo "Running 'echo' and 'pwd':"
echo "Hello from test1.sh"
pwd

echo "Attempting to 'cd dir-DNE'..."
cd dir-DNE
pwd

echo "Which command for 'ls':"
which ls

echo "There won't be a next line if exit works properly."
exit
echo "If you see this line, 'exit' didn't stop the script."