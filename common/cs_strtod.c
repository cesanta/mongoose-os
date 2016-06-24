#include <ctype.h>
#include <math.h>

#include <stdlib.h>

int cs_strncasecmp(const char *s1, const char *s2, size_t n) {
  if (n == 0) {
    return 0;
  }

  while (n-- != 0 && tolower((int) *s1) == tolower((int) *s2)) {
    if (n == 0 || *s1 == '\0' || *s2 == '\0') {
      break;
    }
    s1++;
    s2++;
  }

  return tolower(*(unsigned char *) s1) - tolower(*(unsigned char *) s2);
}

/*
 * based on Source:
 * https://github.com/anakod/Sming/blob/master/Sming/system/stringconversion.cpp#L93
 */

double cs_strtod(const char *str, char **endptr) {
  double result = 0.0;
  char c;
  const char *str_start;
  struct {
    unsigned neg : 1;        /* result is negative */
    unsigned decimals : 1;   /* parsing decimal part */
    unsigned is_exp : 1;     /* parsing exponent like e+5 */
    unsigned is_exp_neg : 1; /* exponent is negative */
  } flags = {0, 0, 0, 0};

  while (isspace((int) *str)) {
    str++;
  }

  if (*str == 0) {
    /* only space in str? */
    if (endptr != 0) *endptr = (char *) str;
    return result;
  }

  /* Handle leading plus/minus signs */
  while (*str == '-' || *str == '+') {
    if (*str == '-') {
      flags.neg = !flags.neg;
    }
    str++;
  }

  if (cs_strncasecmp(str, "NaN", 3) == 0) {
    if (endptr != 0) *endptr = (char *) str + 3;
    return NAN;
  }

  if (cs_strncasecmp(str, "INF", 3) == 0) {
    str += 3;
    if (cs_strncasecmp(str, "INITY", 5) == 0) str += 5;
    if (endptr != 0) *endptr = (char *) str;
    return flags.neg ? -INFINITY : INFINITY;
  }

  str_start = str;

  if (*str == '0' && (*(str + 1) == 'x' || *(str + 1) == 'X')) {
    /* base 16 */
    str += 2;
    while ((c = tolower((int) *str))) {
      int d;
      if (c >= '0' && c <= '9') {
        d = c - '0';
      } else if (c >= 'a' && c <= 'f') {
        d = 10 + (c - 'a');
      } else {
        break;
      }
      result = 16 * result + d;
      str++;
    }
  } else if (*str == '0' && (*(str + 1) == 'b' || *(str + 1) == 'B')) {
    /* base 2 */
    str += 2;
    while ((c = *str)) {
      int d = c - '0';
      if (c != '0' && c != '1') break;
      result = 2 * result + d;
      str++;
    }
  } else if (*str == '0' && *(str + 1) >= '0' && *(str + 1) <= '7') {
    /* base 8 */
    while ((c = *str)) {
      int d = c - '0';
      if (c < '0' || c > '7') {
        /* fallback to base 10 */
        str = str_start;
        break;
      }
      result = 8 * result + d;
      str++;
    }
  }

  if (str == str_start) {
    /* base 10 */

    /* exponent specified explicitly, like in 3e-5, exponent is -5 */
    int exp = 0;
    /* exponent calculated from dot, like in 1.23, exponent is -2 */
    int exp_dot = 0;

    result = 0;

    while ((c = *str)) {
      int d;

      if (c == '.') {
        if (!flags.decimals) {
          /* going to parse decimal part */
          flags.decimals = 1;
          str++;
          continue;
        } else {
          /* non-expected dot: assume number data is over */
          break;
        }
      } else if (c == 'e' || c == 'E') {
        /* going to parse exponent part */
        flags.is_exp = 1;
        str++;
        c = *str;

        /* check sign of the exponent */
        if (c == '-' || c == '+') {
          if (c == '-') {
            flags.is_exp_neg = 1;
          }
          str++;
        }

        continue;
      }

      d = c - '0';
      if (d < 0 || d > 9) {
        break;
      }

      if (!flags.is_exp) {
        /* apply current digit to the result */
        result = 10 * result + d;
        if (flags.decimals) {
          exp_dot--;
        }
      } else {
        /* apply current digit to the exponent */
        if (flags.is_exp_neg) {
          if (exp > -1022) {
            exp = 10 * exp - d;
          }
        } else {
          if (exp < 1023) {
            exp = 10 * exp + d;
          }
        }
      }

      str++;
    }

    exp += exp_dot;

    /*
     * TODO(dfrank): it probably makes sense not to adjust intermediate `double
     * result`, but build double number accordingly to IEEE 754 from taken
     * (integer) mantissa, exponent and sign. That would work faster, and we
     * can avoid any possible round errors.
     */

    /* if exponent is non-zero, apply it */
    if (exp != 0) {
      if (exp < 0) {
        while (exp++ != 0) {
          result /= 10;
        }
      } else {
        while (exp-- != 0) {
          result *= 10;
        }
      }
    }
  }

  if (flags.neg) {
    result = -result;
  }

  if (endptr != 0) {
    *endptr = (char *) str;
  }

  return result;
}
