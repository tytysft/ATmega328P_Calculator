#include <avr/io.h>
#include <util/delay.h>
#include "i2c_master.h"
#include "liquid_crystal_i2c.h"

char buffer[16];
uint8_t buffer_len = 0;

void int_to_str(int num, char* str) {
    if (num == 0) {
        str[0] = '0';
        str[1] = '\0';
        return;
    }
    
    char temp[16];
    uint8_t i = 0;
    uint8_t negative = 0;
    
    if (num < 0) {
        negative = 1;
        num = -num;
    }
    
    while (num > 0) {
        temp[i++] = '0' + (num % 10);
        num /= 10;
    }
    
    uint8_t j = 0;
    if (negative) str[j++] = '-';
    
    for (int8_t k = i - 1; k >= 0; k--) {
        str[j++] = temp[k];
    }
    str[j] = '\0';
}

void add_char(LiquidCrystalDevice_t* lcd, char c) {
    if (buffer_len < 15) {
        buffer[buffer_len++] = c;
        buffer[buffer_len] = '\0';
        lq_clear(lcd);
        lq_print(lcd, buffer);
    }
}

void delete_char(LiquidCrystalDevice_t* lcd) {
    if (buffer_len > 0) {
        buffer[--buffer_len] = '\0';
        lq_clear(lcd);
        lq_print(lcd, buffer);
    }
}

void clear_all(LiquidCrystalDevice_t* lcd) {
    buffer_len = 0;
    buffer[0] = '\0';
    lq_clear(lcd);
}

void calculate(LiquidCrystalDevice_t* lcd) {
    if (buffer_len == 0) return;
    
    int a = 0, b = 0;
    char op = 0;
    uint8_t i = 0;
    
    while (i < buffer_len && buffer[i] >= '0' && buffer[i] <= '9') {
        a = a * 10 + (buffer[i] - '0');
        i++;
    }
    
    if (i < buffer_len && (buffer[i] == '+' || buffer[i] == '-' || buffer[i] == '*' || buffer[i] == '/')) {
        op = buffer[i];
        i++;
    } else {
        lq_clear(lcd);
        lq_print(lcd, "Error: no op");
        _delay_ms(1000);
        clear_all(lcd);
        return;
    }
    
    while (i < buffer_len && buffer[i] >= '0' && buffer[i] <= '9') {
        b = b * 10 + (buffer[i] - '0');
        i++;
    }
    
    int result = 0;
    switch(op) {
        case '+': result = a + b; break;
        case '-': result = a - b; break;
        case '*': result = a * b; break;
        case '/': 
            if (b == 0) {
                lq_clear(lcd);
                lq_print(lcd, "Error: div0");
                _delay_ms(1000);
                clear_all(lcd);
                return;
            }
            result = a / b; 
            break;
        default:
            lq_clear(lcd);
            lq_print(lcd, "Error: op?");
            _delay_ms(1000);
            clear_all(lcd);
            return;
    }
    
    clear_all(lcd);
    lq_print(lcd, "=");
    
    char result_str[16];
    int_to_str(result, result_str);
    lq_print(lcd, result_str);
    _delay_ms(1500);
    clear_all(lcd);
}

uint8_t scan_keypad(void) {
    const char keys[4][4] = {
        {'0','1','2','3'},
        {'4','5','6','7'},
        {'8','9','='},
        {'+','-','*','/'}
    };
    
    for (uint8_t row = 0; row < 4; row++) {
        PORTD |= (1 << PD4) | (1 << PD5) | (1 << PD6) | (1 << PD7);
        
        switch(row) {
            case 0: PORTD &= ~(1 << PD4); break;
            case 1: PORTD &= ~(1 << PD5); break;
            case 2: PORTD &= ~(1 << PD6); break;
            case 3: PORTD &= ~(1 << PD7); break;
        }
        
        _delay_us(50);
        
        if (!(PIND & (1 << PD0))) return keys[row][0];
        if (!(PIND & (1 << PD1))) return keys[row][1];
        if (!(PIND & (1 << PD2))) return keys[row][2];
        if (!(PIND & (1 << PD3))) return keys[row][3];
    }
    return 0;
}

int main(void) {
    DDRD |= (1 << PD4) | (1 << PD5) | (1 << PD6) | (1 << PD7);
    
    DDRD &= ~((1 << PD0) | (1 << PD1) | (1 << PD2) | (1 << PD3));
    PORTD |= (1 << PD0) | (1 << PD1) | (1 << PD2) | (1 << PD3);
    
    i2c_master_init();
    _delay_ms(100);
    
    LiquidCrystalDevice_t lcd = lq_init(0x27, 16, 2, LCD_5x8DOTS);
    _delay_ms(50);
    
    lq_turnOnBacklight(&lcd);
    lq_clear(&lcd);
    lq_print(&lcd, "Calculator ready");
    _delay_ms(1000);
    lq_clear(&lcd);
    
    uint8_t last_key = 0;
    
    while(1) {
        uint8_t key = scan_keypad();
        
        if (key != 0 && key != last_key) {
            if (key == '=') {
                calculate(&lcd);
            } else if (key == '+' || key == '-' || key == '*' || key == '/') {
                add_char(&lcd, key);
            } else if (key >= '0' && key <= '9') {
                add_char(&lcd, key);
            }
            
            last_key = key;
            _delay_ms(150);
        } else if (key == 0) {
            last_key = 0;
        }
        
        _delay_ms(20);
    }
}
