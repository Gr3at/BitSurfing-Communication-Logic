echo "iterate through files and remove first 5 lines"

for f in *.txt; do tail -n +6 "$f" > temp.txt; cp temp.txt "$f"; done
rm temp.txt
