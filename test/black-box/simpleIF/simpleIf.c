int main() {
  int w = 3;
  for (int i = 0; i < 200; i++) {
    if (i % 2)
      w *= i;
    else
      w *= i*i;
  }
  return w;
}
