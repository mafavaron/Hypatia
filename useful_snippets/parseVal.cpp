// Convert a string to a floating point value, or to an invalid conventional value
// if the special symbol NAN is encountered.

float parseVal(const char* sString, const float invalid=-9999.9f) {
  if(strcmp(sString, "NAN") == 0) {
    return(invalid);
  }
  else {
    return(atof(sString));
  }
};
