#include <QKeyEvent>
#include <QDebug>
#include "keycapturer.h"
#include "qkeycode/qkeycode.h"
#include "qkeycode/chromium/keycode_converter.h"
#include "qkeycode/chromium/dom_code.h"
#include "../esp32/main/keyboard.h"

#include <QMap>

using namespace qkeycode;
using namespace qkeycode::ui;

#if 0
#define SPECT_KEYIDX_SHIFT SKEY(0,0)
#define SPECT_KEYIDX_Z     SKEY(0,1)
#define SPECT_KEYIDX_X     SKEY(0,2)
#define SPECT_KEYIDX_C     SKEY(0,3)
#define SPECT_KEYIDX_V     SKEY(0,4)

#define SPECT_KEYIDX_A     SKEY(1,0)
#define SPECT_KEYIDX_S     SKEY(1,1)
#define SPECT_KEYIDX_D     SKEY(1,2)
#define SPECT_KEYIDX_F     SKEY(1,3)
#define SPECT_KEYIDX_G     SKEY(1,4)

#define SPECT_KEYIDX_Q     SKEY(2,0)
#define SPECT_KEYIDX_W     SKEY(2,1)
#define SPECT_KEYIDX_E     SKEY(2,2)
#define SPECT_KEYIDX_R     SKEY(2,3)
#define SPECT_KEYIDX_T     SKEY(2,4)

#define SPECT_KEYIDX_1     SKEY(3,0)
#define SPECT_KEYIDX_2     SKEY(3,1)
#define SPECT_KEYIDX_3     SKEY(3,2)
#define SPECT_KEYIDX_4     SKEY(3,3)
#define SPECT_KEYIDX_5     SKEY(3,4)

#define SPECT_KEYIDX_0     SKEY(4,0)
#define SPECT_KEYIDX_9     SKEY(4,1)
#define SPECT_KEYIDX_8     SKEY(4,2)
#define SPECT_KEYIDX_7     SKEY(4,3)
#define SPECT_KEYIDX_6     SKEY(4,4)

#define SPECT_KEYIDX_P     SKEY(5,0)
#define SPECT_KEYIDX_O     SKEY(5,1)
#define SPECT_KEYIDX_I     SKEY(5,2)
#define SPECT_KEYIDX_U     SKEY(5,3)
#define SPECT_KEYIDX_Y     SKEY(5,4)

#define SPECT_KEYIDX_ENTER SKEY(6,0)
#define SPECT_KEYIDX_L     SKEY(6,1)
#define SPECT_KEYIDX_K     SKEY(6,2)
#define SPECT_KEYIDX_J     SKEY(6,3)
#define SPECT_KEYIDX_H     SKEY(6,4)

#define SPECT_KEYIDX_SPACE SKEY(7,0)
#define SPECT_KEYIDX_SYM   SKEY(7,1)
#define SPECT_KEYIDX_M     SKEY(7,2)
#define SPECT_KEYIDX_N     SKEY(7,3)
#define SPECT_KEYIDX_B     SKEY(7,4)
#endif

static const QMap<KeyCode, uint64_t> key_mappings {
    { KeyCode::DIGIT0, (1UL<<SPECT_KEYIDX_0) },
    { KeyCode::DIGIT1, (1UL<<SPECT_KEYIDX_1) },
    { KeyCode::DIGIT2, (1UL<<SPECT_KEYIDX_2) },
    { KeyCode::DIGIT3, (1UL<<SPECT_KEYIDX_3) },
    { KeyCode::DIGIT4, (1UL<<SPECT_KEYIDX_4) },
    { KeyCode::DIGIT5, (1UL<<SPECT_KEYIDX_5) },
    { KeyCode::DIGIT6, (1UL<<SPECT_KEYIDX_6) },
    { KeyCode::DIGIT7, (1UL<<SPECT_KEYIDX_7) },
    { KeyCode::DIGIT8, (1UL<<SPECT_KEYIDX_8) },
    { KeyCode::DIGIT9, (1UL<<SPECT_KEYIDX_9) },
    { KeyCode::US_A, (1UL<<SPECT_KEYIDX_A) },
    { KeyCode::US_B, (1UL<<SPECT_KEYIDX_B) },
    { KeyCode::US_C, (1UL<<SPECT_KEYIDX_C) },
    { KeyCode::US_D, (1UL<<SPECT_KEYIDX_D) },
    { KeyCode::US_E, (1UL<<SPECT_KEYIDX_E) },
    { KeyCode::US_F, (1UL<<SPECT_KEYIDX_F) },
    { KeyCode::US_G, (1UL<<SPECT_KEYIDX_G) },
    { KeyCode::US_H, (1UL<<SPECT_KEYIDX_H) },
    { KeyCode::US_I, (1UL<<SPECT_KEYIDX_I) },
    { KeyCode::US_J, (1UL<<SPECT_KEYIDX_J) },
    { KeyCode::US_K, (1UL<<SPECT_KEYIDX_K) },
    { KeyCode::US_L, (1UL<<SPECT_KEYIDX_L) },
    { KeyCode::US_M, (1UL<<SPECT_KEYIDX_M) },
    { KeyCode::US_N, (1UL<<SPECT_KEYIDX_N) },
    { KeyCode::US_O, (1UL<<SPECT_KEYIDX_O) },
    { KeyCode::US_P, (1UL<<SPECT_KEYIDX_P) },
    { KeyCode::US_Q, (1UL<<SPECT_KEYIDX_Q) },
    { KeyCode::US_R, (1UL<<SPECT_KEYIDX_R) },
    { KeyCode::US_S, (1UL<<SPECT_KEYIDX_S) },
    { KeyCode::US_T, (1UL<<SPECT_KEYIDX_T) },
    { KeyCode::US_U, (1UL<<SPECT_KEYIDX_U) },
    { KeyCode::US_V, (1UL<<SPECT_KEYIDX_V) },
    { KeyCode::US_W, (1UL<<SPECT_KEYIDX_W) },
    { KeyCode::US_X, (1UL<<SPECT_KEYIDX_X) },
    { KeyCode::US_Y, (1UL<<SPECT_KEYIDX_Y) },
    { KeyCode::US_Z, (1UL<<SPECT_KEYIDX_Z) },

    { KeyCode::ENTER, (1UL<<SPECT_KEYIDX_ENTER) },
    { KeyCode::SPACE, (1UL<<SPECT_KEYIDX_SPACE) },
    // Shifts
    { KeyCode::SHIFT_LEFT, (1UL<<SPECT_KEYIDX_SHIFT) },
    { KeyCode::SHIFT_RIGHT, (1UL<<SPECT_KEYIDX_SHIFT) },
    { KeyCode::ALT_LEFT, (1UL<<SPECT_KEYIDX_SYM) },
    { KeyCode::ALT_RIGHT, (1UL<<SPECT_KEYIDX_SYM) },
    // Composite keys
    { KeyCode::ESCAPE, (1UL<<SPECT_KEYIDX_SPACE)|(1UL<<SPECT_KEYIDX_SHIFT) },
    { KeyCode::BACKSPACE, (1UL<<SPECT_KEYIDX_0)|(1UL<<SPECT_KEYIDX_SHIFT) },
    { KeyCode::ARROW_LEFT, (1UL<<SPECT_KEYIDX_5)|(1UL<<SPECT_KEYIDX_SHIFT) },
    { KeyCode::ARROW_DOWN, (1UL<<SPECT_KEYIDX_6)|(1UL<<SPECT_KEYIDX_SHIFT) },
    { KeyCode::ARROW_UP, (1UL<<SPECT_KEYIDX_7)|(1UL<<SPECT_KEYIDX_SHIFT) },
    { KeyCode::ARROW_RIGHT, (1UL<<SPECT_KEYIDX_8)|(1UL<<SPECT_KEYIDX_SHIFT) },
};


KeyCapturer::KeyCapturer()
{
    m_keys = 0;
}

bool KeyCapturer::keyPressEvent(QKeyEvent *event)
{
    QMap<KeyCode, uint64_t>::const_iterator k;
    qkeycode::KeyCode c = qkeycode::toKeycode(event);
    
    k = key_mappings.find(c);
    if (k==key_mappings.end())
        return false;

    uint64_t oldkeys = m_keys;
    m_keys |= *k;
    if (m_keys!=oldkeys)
        emit keyChanged(m_keys);

    return true;
}

bool KeyCapturer::keyReleaseEvent(QKeyEvent *event)
{
    QMap<KeyCode, uint64_t>::const_iterator k;
    qkeycode::KeyCode c = qkeycode::toKeycode(event);
    
    k = key_mappings.find(c);
    if (k==key_mappings.end())
        return false;

    uint64_t oldkeys = m_keys;
    m_keys &= ~(*k);
    if (m_keys!=oldkeys)
        emit keyChanged(m_keys);

    return true;
}

bool KeyCapturer::eventFilter(QObject *obj, QEvent *event)
{
    QKeyEvent *k = dynamic_cast<QKeyEvent*>(event);

    if (k) {
        switch (k->type()) {
        case QEvent::KeyPress:
            return keyPressEvent(k);
        case QEvent::KeyRelease:
            return keyReleaseEvent(k);
        default:
            break;
        }
    }
    QFocusEvent *ev = dynamic_cast<QFocusEvent*>(event);
    if (ev) {
        if (ev->type()==QEvent::FocusOut) {
            uint64_t oldkeys = m_keys;
            m_keys = 0;
            if (m_keys!=oldkeys)
                emit keyChanged(m_keys);
        }
    }

    return false;
}


