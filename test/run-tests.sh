black-box-test() {
  cd black-box
  ./run.sh
  cd ..;
}

echo "========= RAin Test Suite ========="
if [ $# -eq 0 ]; then
  echo "> Executing black-boxes tests..."
  black-box-test;
  echo "..done."
fi
echo "==================================="
