/*printf для ардуино*/


int ardprintf(const char *str, ...)
{
  int i, count=0, j=0, flag=0;
  String temp = "";
  bool enter = false;
  
  for(i=0; str[i]!='\0'; i++)  if(str[i]=='%')  count++;
  va_list argv;
  va_start(argv, count);
  for(i=0,j=0; str[i]!='\0';i++)
  {
    if(str[i]=='%')
    {
      Serial.print(temp);
      j=0;
      temp = "";

        switch(str[++i])
        {
          case 'd': Serial.print(va_arg(argv, int));
                    break;
          case 'l': Serial.print(va_arg(argv, long));
                    break;
          case 'f': Serial.print(va_arg(argv, double));
                    break;
          case 'c': Serial.print((char)va_arg(argv, int));
                    break;
          case 's': Serial.print(va_arg(argv, const char));
                    break;
          case 'n': enter = true;
                    break;
          default:  ;
        };
      }
    else 
    {
      temp += str[i];
      j++;
    }
  };
  
  //
  if (!enter) {
    Serial.println();
    
  //
  } else {
    
  }
  // (условие) ? false : true; 
  return count + 1;
}

