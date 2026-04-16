#include <stdio.h>
#include <sys/time.h>
#include <time.h>

int main() {
    struct timeval tv;
    char buffer[30];
    
    // 1. Get current time
    gettimeofday(&tv, NULL);

    time_t seconds = (time_t)tv.tv_sec;

    
    // 2. Format calendar time (up to seconds)
    struct tm* tm_info = localtime(&seconds);
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm_info);
    
    // 3. Append microseconds (.06d ensures 6 places with leading zeros)
    printf("%s.%06ld\n", buffer, tv.tv_usec);
    
    return 0;
}
// Output example: 2026-04-15 14:30:05.123456
