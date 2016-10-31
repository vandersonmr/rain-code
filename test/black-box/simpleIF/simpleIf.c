int main() {
  int w = 3;
  for (int i = 0; i < 200; i++) {
        if (i < 60)
          w *= i;
        else if (i > 60)
          w *= i*i;
  }
  return w;
}
