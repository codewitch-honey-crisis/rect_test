#include <Arduino.h>
#include <htcw_data.hpp>
#include <ttgo.hpp>
using namespace arduino;
using namespace gfx;
using rect_list_t = data::simple_vector<srect16>;
typedef struct split_rect {
    srect16 rects[4];
    size_t count;
} split_rect_t;
using split_rect_stack_t = data::simple_stack<split_rect_t>;
const size_t buffer_size = 8 * 1024;
const float max_area = ((float)buffer_size) / ((float)decltype(lcd)::pixel_type::bit_depth / 8.0f);
const float widthf = sqrtf(max_area);
const int width = floorf(widthf);
const size_t height = width + (roundf(widthf) != (float)width);
rect_list_t control_rects;
rect_list_t fill_rects;
split_rect_stack_t split_stack;
bool combine_fill_rect(srect16& r) {
    bool found = false;
    for(auto fr : fill_rects) {
        if(fr==r || fr.contains(r)) {
            found = true;
            break;
        } else {
            uint32_t area = (uint32_t)(fr.area());
            area+=(uint32_t)(r.area());
            if(area<=max_area) {
                if(fr.x1==r.x1&&fr.x2==r.x2) {
                    if(fr.y1==r.y2+1) { // fr extends from the bottom of r
                        fr.y1 = r.y1;
                        found = true;
                        r=fr;
                    } else if(fr.y2==r.y1-1) { // fr extends from the top of r
                        fr.y2=r.y2;
                        found = true;
                        r=fr;
                    }
                }
                if(fr.y1==r.y1&&fr.y2==r.y2) {
                    if(fr.x1==r.x2+1) { // fr extends to the right of r
                        fr.x1 = r.x1;
                        found = true;
                        r=fr;
                    } else if(fr.x2==r.x1-1) { // fr extends to the left of r
                        fr.x2=r.x2;
                        found = true;
                        r=fr;
                    }
                }
                if(found) {
                    Serial.println("Combined rects");
                    break;
                }
            }
        }
    }
    if(!found) {
        fill_rects.push_back(r);
        return true;
    }
    return false;
}
void draw_rects() {
    for(const auto cr : control_rects) {
        draw::rectangle(lcd, cr, color_t::red);
    }
    for(const auto fr : fill_rects) {
        draw::rectangle(lcd,fr,color_t::white);
    }
}
void make_fill_rects() {
    fill_rects.clear();
    // fill in the areas where the other rects weren't
    // up to 128x128 (@ 32kB) in size
    for (int y = 0; y < lcd.dimensions().height; y += height) {
        for (int x = 0; x < lcd.dimensions().width; x += width) {
            bool done = false;
            split_rect_t sr;
            sr.count = 1;
            sr.rects[0] =srect16(spoint16(x, y), ssize16(width, height));
            if(!split_stack.push(sr)) {
                Serial.println("Out of memory");
                return;
            }
            while(split_stack.size()) {
                split_rect_t current = *split_stack.pop();
                for (size_t i = 0; i < current.count; ++i) {
                    srect16 r = current.rects[i].crop((srect16)lcd.bounds());
                    bool intersects = false;
                    for (const auto cr : control_rects) {
                        if (r.intersects(cr) && !r.contains(cr) && !cr.contains(r)) {
                            intersects = true;
                            split_rect_t srn;
                            srn.count = r.split(cr, 4, srn.rects);
                            if (srn.count == 1 && r == *srn.rects) {
                                // just store it as is
                                //combine_fill_rect(r);         
                                fill_rects.push_back(r);
                            } else if(srn.count>0) {
                                split_stack.push(srn);
                            }
                        }
                    }
                    if (!intersects) {
                        // just store it as is
                        //combine_fill_rect(r);
                        fill_rects.push_back(r);
                    }
                }
            }
        }
    }
    split_stack.clear();
}

// this is basically main() in normal C++
void setup() {
    Serial.begin(115200);
    lcd.initialize();
    lcd.rotation(3);
}
void loop() {
    // ensure the backlight stays on
    dimmer.wake();
    // clear the screen
    draw::filled_rectangle(lcd, lcd.bounds(), color_t::black);
    // start from scratch
    control_rects.clear();
    fill_rects.clear();
    // just add some random rects to the screen
    int ctl_count = rand() % 5 + 1;
    rect_list_t new_ctls;
    // tried to keep them from overlapping but it's not working.
    for(int i = 0;i<ctl_count;++i) {
        // make a random rectangle
        srect16 ctl_rect(rand()%lcd.dimensions().width,rand()%lcd.dimensions().height,
                rand()%lcd.dimensions().width,rand()%lcd.dimensions().height);
        // ensure the rect point 1 is the upper left
        ctl_rect.normalize_inplace();
        // ensure this rect doesn't overlap with any we've already
        // added
        bool skip = false;
        for(const auto cmp : control_rects) {
            if(ctl_rect.intersects(cmp)) {
                skip = true;
                break;
            }
        }
        // add it to the "controls"
        if(!skip) {
            control_rects.push_back(ctl_rect);
        }
    }
    // fill the remainder with rects
    make_fill_rects();
    // draw all the remainder rects
    draw_rects();
    // wait 5 seconds
    delay(5 * 1000);
}