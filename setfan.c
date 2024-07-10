#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

/* This program allows to easily set the fan speed with `setfan speed`
 * Speed can be from 0 (off) to 7 (max) and `auto`.
 * 
 * If the speed is set to 0, the CPU temperature is monitored and the
 * fan speed is set to auto if temperture exceeds 70° for 10 seconds.
 *
 * Requires libsensors
 *
 * Hint: Use sticky bit to automatically run as root
 */


// allow turning off the fan?
#define ALLOW_0

#ifdef ALLOW_0
#include <sensors/sensors.h>
#endif

void set_fan_speed(const char *speed) {
  FILE *fan = fopen("/proc/acpi/ibm/fan", "w");
  if (fan == NULL) {
    printf("couldn't open /proc/acpi/ibm/fan %d %s\n", errno, strerror(errno));
    exit(1);
  }
  fprintf(fan, "level %s\n", speed);
  fclose(fan);  
}

void sig_handler(int signo)
{
  set_fan_speed("auto");
  _exit(signo);
}

void monitor_temp() {
#ifdef ALLOW_0
  printf("monitoring temperature...\n");
  
  int fanspeed = 0;
  int temp_crit = 0;
  
  sensors_init(NULL);
  
  int chip_nr = 0, i = 0, j = 0;
  double val, highest_val = 0;
  const sensors_chip_name *chip = NULL;
  const sensors_feature *feature;
  const sensors_subfeature *subfeature;
  
  while ((chip = sensors_get_detected_chips(NULL, &chip_nr))) {
    if (strcmp("coretemp", chip->prefix) == 0) {
      break;
    }
  }
  
  if (chip == NULL) {
    printf("Couldn't find coretemp sensors\n");
    set_fan_speed("auto");
    printf("Set fan speed to auto!");
    exit(1);
  }
  
  while (1) {
    highest_val = 0;
    i = 0;
    j = 0;
    
    while ((feature = sensors_get_features(chip, &i))) {
      //printf("%s %d  ", feature->name, feature->type);
      subfeature = sensors_get_subfeature(chip, feature, SENSORS_SUBFEATURE_TEMP_INPUT);
      
      if (subfeature == NULL)
        continue;
        
      if (sensors_get_value(chip, subfeature->number, &val) == 0) {
        //printf("%+6.1f%s  ", val, "°");
        if (val > highest_val) {
          highest_val = val;
        }
      }
      //puts("");
    }
    printf("\rtemp: %+6.1f°C", highest_val);
    fflush(stdout);
    
    if (highest_val > 70) {
      if (temp_crit) {
        set_fan_speed("auto");
        printf("\nSet fan speed to auto!");
        exit(0);
      }
      temp_crit = 1;
    }
    else {
      temp_crit = 0;
    }
    
    sleep(5);
  }

#endif
}


int main(int argc, char *argv[]) {
  char speed[10];
  
  if (argc > 1 && strcmp("test", argv[1]) == 0) {
    monitor_temp() ;
    return 1;
  } 

  if (setuid(0) != 0) {
    printf("I must be root\n");
    return 1;
  }
  
  strcpy(speed, "auto");  //default value

  if (argc > 1) {
    if (strcmp("test", argv[1]) == 0) {
      monitor_temp() ;
      return 1;
    }    

#ifdef ALLOW_0
    if (argv[1][0] >= '0' && argv[1][0] <= '7' || strcmp("auto", argv[1]) == 0) {
#else
    if (argv[1][0] >= '1' && argv[1][0] <= '7' || strcmp("auto", argv[1]) == 0) {
#endif
      strcpy(speed, argv[1]);
    }
    else {
      printf("invalid argument %s\n", argv[1]);
      return 1;
    }
  }
  
  printf("settings speed to %s\n", speed);
  
  if (speed[0] == '0') {
    if (signal(SIGABRT, sig_handler) == SIG_ERR ||
        signal(SIGFPE,  sig_handler) == SIG_ERR ||
        signal(SIGHUP,  sig_handler) == SIG_ERR ||
        signal(SIGILL,  sig_handler) == SIG_ERR ||
        signal(SIGPIPE, sig_handler) == SIG_ERR ||
        signal(SIGQUIT, sig_handler) == SIG_ERR ||
        signal(SIGSEGV, sig_handler) == SIG_ERR ||
        signal(SIGTERM, sig_handler) == SIG_ERR ||
        signal(SIGINT, sig_handler) == SIG_ERR) {
      printf("Couldn't add signal handler, not setting fan speed to 0\n");
      exit(1);   
    }
  }
  
  set_fan_speed(speed);
  
  if (speed[0] != '0') {
    return 0;
  }
  
  monitor_temp();
  
  return 0;
}
