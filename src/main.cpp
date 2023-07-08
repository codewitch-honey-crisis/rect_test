#include <Arduino.h>
#include <ttgo.hpp>
#include <htcw_data.hpp>
using namespace arduino;
using namespace gfx;
using rect_list_t = data::simple_vector<srect16>;

rect_list_t control_rects;
rect_list_t fill_rects;
void make_fill_rects() {
    // first pass

    // fill in the areas where the other rects weren't
    // up to 128x128 in size
    for(int y=0;y<lcd.dimensions().height;y+=128) {
        for(int x=0;x<lcd.dimensions().width;x+=128) {
            srect16 r(spoint16(x,y),ssize16(128,128));
            for(const srect16* it = control_rects.cbegin();it!=control_rects.cend();++it) {
                if(r.intersects(*it) || r.contains(*it) || it->contains(r)) {
                    // here we punch a rect sized hole in another rect
                    // which returns up to 4 rects in response
                    srect16 out_rects[4];
                    size_t out_size=r.split(*it,4,out_rects);
                    for(size_t i = 0;i<out_size;++i) {
                        r = out_rects[i];           
                        // make sure within bounds
                        r=r.crop((srect16)lcd.bounds());
                        // draw it
                        draw::rectangle(lcd,r,color_t::white);    
                        fill_rects.push_back(r);
                    }
                    break;
                } else {
                    // make sure it's within screen bounds
                    r=r.crop((srect16)lcd.bounds());
                    // just draw it and store it as is
                    draw::rectangle(lcd,r,color_t::white); 
                    fill_rects.push_back(r);
                }
            }
        }
    }
}

// this is basically main() in normal C++
void setup() {
    Serial.begin(115200);
    lcd.initialize();
    lcd.rotation(3);
    draw::filled_rectangle(lcd,lcd.bounds(),color_t::black);
    // just add some random rects to the screen
    int ctl_count = rand()%5;
    for(int i = 0;i<ctl_count;++i) {
        srect16 r(
            spoint16(rand()%lcd.dimensions().width,rand()%lcd.dimensions().height),
            spoint16(rand()%lcd.dimensions().width,rand()%lcd.dimensions().height));
        r.normalize_inplace();
        draw::rectangle(lcd,r,color_t::red);
        control_rects.push_back(r);
    }
    make_fill_rects();
}
void loop() {
    dimmer.wake();
}