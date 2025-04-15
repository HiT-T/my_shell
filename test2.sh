echo "===== Test 2: Redirection & Pipes ====="

echo "Reading from input.txt and piping to 'grep':"
cat input.txt | grep pattern

echo "Listing files to out.txt..."
ls > out.txt

echo "Contents of out.txt:"
cat out.txt

echo "Piping 'pwd' into 'wc -c':"
pwd | wc -c