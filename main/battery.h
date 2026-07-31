#pragma once

#define BATTERY_CELLS_NUMBER 2

void battery_init(void);

float battery_get_voltage(void);
float battery_get_cell_voltage(void);

int battery_get_percentage(void);

void battery_set_cells(int cells);
