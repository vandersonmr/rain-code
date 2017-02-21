int main()
{
  int BUBBLE = 100;
  int myArray[100];
  int i, j;
  int temp = 0;
  int num;

  //fill array
  for (i = 0; i < BUBBLE; i ++) {
    num = i*3 % BUBBLE + 1;
    myArray[i] = num;
  }

  //sort array
  for(i = 0; i < BUBBLE; i++)	{
    for (j = 0; j < BUBBLE-1; j++) {
      if (myArray[j] > myArray[j+1]) {
        temp = myArray[j];
        myArray[j] = myArray[j+1];
        myArray[j+1] = temp;
      }
    }/*End inner for loop*/
  }/*End outer for loop*/

  return 0;
}
