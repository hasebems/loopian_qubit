//  Created by Hasebe Masahiko on 2025/07/10.
//  Copyright (c) 2025 Hasebe Masahiko.
//  Released under the MIT license
//  https://opensource.org/licenses/mit-license.php
//
#ifndef QTOUCH_H
#define QTOUCH_H

#include <cstdint>
#include <array>
#include <vector>
#include <functional>
#include <algorithm>
#include <cmath>

#include "constants.h"

// =========================================================
//      Touch Constants
// =========================================================
constexpr uint16_t MAX_PADS = MAX_SENS;
constexpr uint16_t TOUCH_THRESHOLD = 30; // Example threshold for touch point detection
constexpr float CLOSE_RANGE = 3.0f; // 同じタッチと見做される 10msec あたりの動作範囲
constexpr size_t FINGER_RANGE = 3; // Maximum number of touch points
constexpr float HISTERESIS = 0.7f; // Hysteresis value for touch point detection


// =========================================================
//      Pad Class
// =========================================================
class Pad {
    static constexpr size_t MAX_MOVING_AVERAGE = 4; // Number of samples for moving average

    uint16_t    mv_avg_value_;  // 移動平均(MAX_MOVING_AVERAGEで割らない)
    int16_t     diff_from_before_;
    bool        top_flag_;
    uint16_t    past_value_[MAX_MOVING_AVERAGE];
    size_t      past_index_;

//impl Pad
public:
    Pad() : mv_avg_value_(0), 
            diff_from_before_(0),
            top_flag_(false),
            past_value_{},
            past_index_(0) {
        std::fill(past_value_, past_value_ + MAX_MOVING_AVERAGE, 0);
    }

    void set_crnt(uint16_t value) {
        uint16_t crnt_value = value;
        past_value_[past_index_] = crnt_value;
        past_index_ = (past_index_ + 1) % MAX_MOVING_AVERAGE;
        mv_avg_value_ = 0;
        for (size_t i = 0; i < MAX_MOVING_AVERAGE; ++i) {
            mv_avg_value_ += past_value_[i];
        }
    }
    auto get_crnt() -> uint16_t const {
        return mv_avg_value_;
    }
    auto set_diff_from_before(int16_t value_before) -> int16_t{
        diff_from_before_ = value_before - mv_avg_value_;
        return diff_from_before_;
    }
    auto diff_from_before() -> int16_t const {
        return diff_from_before_;
    }
    void note_top_flag() {top_flag_ = true;}
    auto is_top_flag() -> bool const {
        return top_flag_;
    }
};

// =========================================================
//      TouchPoint Class
// =========================================================
// センサーの生値から、実際にどのあたりをタッチしているかを判断し、保持する
class TouchPoint {
    static constexpr uint8_t NEW_NOTE = 0xff;
    static constexpr uint8_t TOUCH_POINT_ERROR = 0xfe;

    float       center_location_;
    int16_t     intensity_;
    uint8_t     real_crnt_note_; // MIDI Note number
    bool        is_updated_;
    bool        is_touched_;
    uint16_t    touching_time_;
    uint16_t    no_update_time_;
    std::function<void(uint8_t, uint8_t, uint8_t)> midi_callback_; // MIDI callback function

// impl TouchPoint
public:
    static constexpr float INIT_VAL = 100.0f;
    static constexpr uint8_t OFFSET_NOTE = 0x18;

    /// Constructor は起動時に最大数分呼ばれる
    TouchPoint() :
        center_location_(INIT_VAL), // Invalid location initially
        intensity_(0),
        real_crnt_note_(0), // Initialize to 0, will be set when a touch is detected
        is_updated_(false),
        is_touched_(false),
        touching_time_(0),
        no_update_time_(0),
        midi_callback_(nullptr) {}

    /// 新しいタッチポイントを作成する
    void new_touch(float location, int16_t intensity, std::function<void(uint8_t, uint8_t, uint8_t)> callback) {
        uint8_t crnt_note = new_location(NEW_NOTE, location);
        if (crnt_note == TOUCH_POINT_ERROR) {
            return;
        }
        center_location_ = location;
        real_crnt_note_ = crnt_note; // Set the current note
        intensity_ = intensity;
        is_updated_ = true;
        is_touched_ = true;
        touching_time_ = 0; // Reset the touching time
        midi_callback_ = callback;
        // MIDI Note On
        if (callback) {
            callback(0x90, real_crnt_note_ + OFFSET_NOTE, intensity_to_velocity(intensity_));
        }
    }
    /// タッチポイントが近いかどうかを判断する
    auto is_near_here(float location) const -> bool {
        if (!is_touched_) { return false;}
        if ((center_location_ >= location - CLOSE_RANGE) && (center_location_ <= location + CLOSE_RANGE)) {
            return true;
        }
        return false;
    }
    /// タッチポイントを更新する
    void update_touch(float location, uint16_t intensity) {
        center_location_ = location;
        intensity_ = intensity;
        is_updated_ = true;
        is_touched_ = true;
        uint8_t updated_note = new_location(real_crnt_note_, location);
        if (updated_note == TOUCH_POINT_ERROR) {
            return;
        }
        // MIDI Note On & Off
        if ((midi_callback_) && (updated_note != real_crnt_note_)) {
            midi_callback_(0x90, updated_note + OFFSET_NOTE, intensity_to_velocity(intensity_));
            midi_callback_(0x80, real_crnt_note_ + OFFSET_NOTE, 0x40);
            real_crnt_note_ = updated_note; // Update the current note
        }
    }
    /// タッチポイントが離れたときの処理
    void maybe_released() {
        if ( no_update_time_ + 5 > touching_time_ ) {
            // 5回以上更新がなかったら、タッチポイントを離れたとみなす
            touching_time_ += 1;
            return;
        }
        // MIDI Note Off
        if (midi_callback_) {
            midi_callback_(0x80, real_crnt_note_ + OFFSET_NOTE, 0x40);
        }
        is_touched_ = false;
        center_location_ = INIT_VAL;
        intensity_ = 0;
    }
    auto is_touched() const -> bool {
        return is_touched_;
    }
    auto is_updated() const -> bool {
        return is_updated_;
    }
    auto get_location() const -> float {
        return center_location_;
    }
    auto get_intensity() const -> int16_t {
        return intensity_;
    }
    void clear_updated_flag() {
        touching_time_ += 1;
        no_update_time_ = touching_time_;
        is_updated_ = false;
    }

private:
    /// crnt_note : 0-(MAX_SENS-1) 現在の位置、NEW_NOTE は新規ノート
    auto new_location(uint8_t crnt_note, float location) -> uint8_t {
        if (location < 0.0f) {
            location = 0.0f; // Ensure location is non-negative
        } else if (location >= static_cast<float>(MAX_SENS - 1)) {
            location = static_cast<float>(MAX_SENS - 1); // Ensure location is within bounds
        }
        if (crnt_note == NEW_NOTE) {
            return static_cast<uint8_t>(std::round(location)); // Round to nearest integer for MIDI note
        } else if (crnt_note < MAX_SENS) {
            if ((location > static_cast<float>(crnt_note) + HISTERESIS) ||
                (location < static_cast<float>(crnt_note) - HISTERESIS)) {
                // histeresis
                return static_cast<uint8_t>(std::round(location));
            } else {
                return crnt_note; // No change in note
            }
        } else {
            // Invalid note number, return TOUCH_POINT_ERROR
            return TOUCH_POINT_ERROR;
        }
    }
    auto intensity_to_velocity(int16_t intensity) const -> uint8_t {
        // Convert intensity to MIDI velocity (0-127)
        if (intensity < 0) {
            return 0; // No touch
        } else if (intensity > 255) {
            return 255; // Max MIDI velocity
        }
        return static_cast<uint8_t>(100 + (intensity >> 4));
    }
};

// =========================================================
//      QubitTouch Class
// =========================================================
// Qubit 全体のタッチを管理するクラス
class QubitTouch {
    std::array<Pad, MAX_PADS> pads_;    // パッドの状態を保持する配列
    std::array<TouchPoint, MAX_TOUCH_POINTS> touch_points_; // Store detected touch points
    std::function<void(uint8_t, uint8_t, uint8_t)> midi_callback_; // MIDI callback function
    size_t touch_count_ = 0; // Current number of touch points
    int16_t debug = 0;

// impl QubitTouch
public:
    QubitTouch(std::function<void(uint8_t, uint8_t, uint8_t)> cb) : 
        pads_{},
        touch_points_{},
        midi_callback_(cb),
        touch_count_(0) {
    }

    /// タッチポイントの数を取得する
    auto deb_val() const -> int16_t {
        return debug;
    }
    /// パッドの値を設定する
    void set_value(size_t pad_num, uint16_t value) {
        pads_[pad_num].set_crnt(value);
    }
    /// パッドの値を取得する
    auto get_value(size_t pad_num) -> uint16_t const {
        return pads_[pad_num].get_crnt();
    }
    /// タッチポイントの数を取得する
    auto get_touch_count() const -> size_t {
        return touch_count_;
    }
    /// タッチポイントの参照を取得する（非const版）
    auto get_touch_point(size_t index) -> TouchPoint& {
        return touch_points_[index];
    }
    /// タッチポイントのconst参照を取得する（const版）
    auto touch_point(size_t index) const -> const TouchPoint& {
        return touch_points_[index];
    }
    /// 指定されたパッドの参照を取得する(マイナス値からMAX_PADSを超えた値を考慮)
    auto proper_pad(int pad_num) -> Pad& {
        while (pad_num < 0) {
            pad_num += MAX_PADS; // Wrap around to ensure valid index
        }
        size_t index = static_cast<size_t>(pad_num);
        return pads_[(pad_num + MAX_PADS) % MAX_PADS]; // Wrap around to ensure valid index
    }
    /// 差分の符号が変化した時、その位置の値がある一定の値以上なら、そこをタッチポイントとする
    void seek_and_update_touch_point() {
        std::array<std::tuple<size_t, float, int16_t>, MAX_TOUCH_POINTS> temp_touch_point;
        temp_touch_point.fill(std::make_tuple(TouchPoint::INIT_VAL, TouchPoint::INIT_VAL, 0));
        size_t temp_index = 0;

        // 1: 全パッドを走査し、差分の符号が変化した箇所をタッチポイントとみなし、temp_touch_point に保存
        int16_t diff_before = 0;
        for (size_t i = 0; i <= MAX_PADS; ++i) {
            Pad& prev_pad = proper_pad(i-1);
            int16_t diff_after = proper_pad(i).set_diff_from_before(prev_pad.get_crnt());
            if ((diff_after > 0 ) && (diff_before < 0)) { // - -> + 変化時
                int16_t value = prev_pad.get_crnt(); // Note the top flag
                if (value > TOUCH_THRESHOLD) { // Example threshold for touch point
                    prev_pad.note_top_flag();
                    std::get<0>(temp_touch_point[temp_index++]) = (i >= 1) ? i - 1 : i - 1 + MAX_PADS;
                    if (temp_index >= MAX_TOUCH_POINTS) {
                        break; // Prevent overflow of touch points
                    }
                }
            }
            diff_before = diff_after;
        }
        touch_count_ = temp_index; // Update the touch count

        // 2: タッチポイントの前後のパッドの値を足し、平均をとってパッドの位置と強度を確定する
        for (int i = 0; i < temp_index; ++i) {
            int16_t sum = 0;
            float locate = 0.0f;
            auto &tpi = temp_touch_point[i];
            int tp = static_cast<int>(std::get<0>(tpi));
            int window_idx = 0; // Initialize window index for averaging
            for (int j = 0; j < FINGER_RANGE*2 + 1; ++j) {
                window_idx = static_cast<int>(j - FINGER_RANGE);
                Pad& neighbor_pad = proper_pad(tp + window_idx);
                int16_t tp_value = neighbor_pad.get_crnt();
                sum += tp_value;
                locate += (tp + window_idx) * tp_value; // Wrap around to ensure valid index
            }
            locate /= sum; // Calculate the average location based on intensity
            std::get<1>(tpi) = locate;
            std::get<2>(tpi) = sum;
        }

        // 3: 各パッドの値を確認し、タッチポイントを更新または追加する
        for (int k = 0; k < temp_index; ++k) {
            auto &tpi = temp_touch_point[k];
            float location = std::get<1>(tpi);
            int16_t intensity = std::get<2>(tpi);
            // 現在のタッチポイントで近いものがあれば、タッチポイントがそこから移動したとみなす
            float nearest = TouchPoint::INIT_VAL;
            TouchPoint* nearest_tp = nullptr;
            for (auto& tp: touch_points_) {
                if (!tp.is_touched() || tp.is_updated()) {
                    continue; // Skip if the touch point is not touched
                }
                float diff = std::abs(tp.get_location() - location);
                if (diff < nearest) {
                    // 近いものがあれば、タッチポイントを更新する
                    nearest = diff;
                    nearest_tp = &tp;
                }
            }
            if (nearest_tp && nearest_tp->is_near_here(location)) {
                // 一番近いタッチポイントが、現在のタッチポイントに近い場合
                nearest_tp->update_touch(location, intensity);
            } else {
                new_touch_point(location, intensity, midi_callback_);
            }
        }

        // 更新のなかったタッチポイントを削除する
        erase_touch_point();
    }
    /// LEDを点灯させるためのコールバック関数をコールする
    void lighten_leds(std::function<void(float, int16_t)> led_callback) {
        bool empty = true;
        for (auto& tp : touch_points_) {
            if (tp.is_touched()) {
                float location = tp.get_location();
                int16_t intensity = tp.get_intensity();
                led_callback(location, intensity);
                empty = false;
            }
        }
        if (empty) {
            // Call the callback with default values if no touch points are active
            led_callback(-1.0f, 0);
        }
    }

private:
    void new_touch_point(float location, uint16_t intensity, std::function<void(uint8_t, uint8_t, uint8_t)> callback) {
        for (auto& tp : touch_points_) {
            if (!tp.is_touched()) {
                tp.new_touch(location, intensity, callback);
                return;
            }
        }
    }
    void erase_touch_point() {
        for (auto& tp : touch_points_) {
            if (!tp.is_updated() && tp.is_touched()) {
                tp.maybe_released();
            } else {
                tp.clear_updated_flag(); // Clear the updated flag for the next cycle
            }
        }
    }
};
#endif // TOUCH_H