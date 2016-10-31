int main() {
  int c = 3;
  for (int i = 1; i < 100; i++)
    for (int j = 1; j < 100; j++)
      for (int w = 1; w < 100; w++)
        c *= i + j;

  return c;
}
