#ifndef CONFIG_H
#define CONFIG_H

typedef enum
{
    FRONT_LEFT = 0,
    FRONT_RIGHT,
    REAR_RIGHT,
    REAR_LEFT,

    LOGICAL_MOTOR_COUNT
} logical_motor_t;

typedef struct
{
    int motor_map[LOGICAL_MOTOR_COUNT];
    int trim[LOGICAL_MOTOR_COUNT];

} drone_config_t;

extern drone_config_t drone_config;

void config_load(void);
void config_save(void);

#endif