#pragma once

void backlight_task(void);
void breathing_interrupt_disable(void);
void breathing_interrupt_enable(void);
bool is_breathing(void);
void backlight_init_ports(void);
void backlight_set(uint8_t level);
