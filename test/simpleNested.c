int main() {
  int w = 3;
  for (int i = 1; i < 100; i++) 
    for (int j = 1; j < 100; j++) 
      w *= i + j;
  return w;
}
