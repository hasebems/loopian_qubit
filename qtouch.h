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

#include "constants.h"

constexpr uint16_t MAX_PADS = MAX_SENS;
constexpr uint16_t TOUCH_THRESHOLD = 10; // Example threshold for touch point detection
constexpr size_t CLOSE_RANGE = 3; // Range for detecting close touch points

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
    int16_t     center_location_;
    int16_t     intensity_;
    bool        is_updated_;
    bool        is_touched_;
    std::function<void(uint8_t, uint8_t, uint8_t)> midi_callback_; // MIDI callback function

// impl TouchPoint
public:
    // デフォルトコンストラクタを追加
    TouchPoint() :
        center_location_(-1),
        intensity_(0),
        is_updated_(false),
        is_touched_(false),
        midi_callback_(nullptr) {}

    void new_touch(uint16_t location, uint16_t intensity, std::function<void(uint8_t, uint8_t, uint8_t)> callback) {
        center_location_ = location;
        intensity_ = intensity;
        is_updated_ = true;
        is_touched_ = true;
        midi_callback_ = callback;
        // MIDI Note On
        if (midi_callback_) {
            midi_callback_(0x90, center_location_, intensity_); // Example MIDI Note On message
        }
    }

    auto is_close_here(uint16_t location) const -> bool {
        if (!is_touched_) { return false;}
        if ((center_location_ >= location - CLOSE_RANGE) && (center_location_ <= location + CLOSE_RANGE)) {
            return true;
        }
        return false;
    }

    void update_touch(uint16_t location, uint16_t intensity) {
        uint16_t previous_location = center_location_;
        center_location_ = location;
        intensity_ = intensity;
        is_updated_ = true;
        is_touched_ = true;
        // MIDI Note On & Off
        midi_callback_(0x90, center_location_, intensity_);
        midi_callback_(0x80, previous_location, 0x40);
    }

    auto is_touched() const -> bool {
        return is_touched_;
    }

    auto is_updated() const -> bool {
        return is_updated_;
    }

    void clear_updated_flag() {
        is_updated_ = false;
    }

    void released() {
        // MIDI Note Off
        if (midi_callback_) {
            midi_callback_(0x80, center_location_, 0x40);
        }
        is_touched_ = false;
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

// impl QubitTouch
public:
    QubitTouch(std::function<void(uint8_t, uint8_t, uint8_t)> cb) : 
        pads_{},
        touch_points_{},
        midi_callback_(cb),
        touch_count_(0) {
    }

    // パッドの値を設定する
    void set_value(size_t pad_num, uint16_t value) {
        pads_[pad_num].set_crnt(value);
    }

    /// タッチポイントの数を取得する
    size_t get_touch_count() const {
        return touch_count_;
    }

    /// 指定されたパッドの参照を取得する(マイナス値からMAX_PADSを超えた値を考慮)
    Pad& proper_pad(size_t pad_num) {
        return pads_[(pad_num + MAX_PADS) % MAX_PADS]; // Wrap around to ensure valid index
    }

    /// 差分の符号が変化した時、その位置の値がある一定の値以上なら、そこをタッチポイントとする
    void seek_and_update_touch_point() {
        int16_t diff_before = 0;
        int16_t temp_touch_point[MAX_TOUCH_POINTS];
        std::fill(temp_touch_point, temp_touch_point + MAX_TOUCH_POINTS, -1); // Initialize all elements to -1
        size_t touch_point_count = 0;

        for (size_t i = 0; i <= MAX_PADS; ++i) {
            Pad& prev_pad = proper_pad(i-1);
            int16_t diff_after = proper_pad(i).set_diff_from_before(prev_pad.get_crnt());
            if ((diff_after > 0 ) && (diff_before < 0)) { // - -> + 変化時
                int16_t value = prev_pad.get_crnt(); // Note the top flag
                if (value > TOUCH_THRESHOLD) { // Example threshold for touch point
                    prev_pad.note_top_flag();
                    if (touch_point_count < MAX_TOUCH_POINTS) {
                        // Store the touch point index, handling wrap-around for negative indices
                        temp_touch_point[touch_point_count++] = (i >= 1) ? i - 1 : i - 1 + MAX_PADS;
                    }
                }
            }
            diff_before = diff_after;
        }
        touch_count_ = touch_point_count;

        // 各パッドの値を確認し、タッチポイントを更新または追加する
        for (auto new_tp : temp_touch_point) {
            if (pads_[new_tp].is_top_flag()) { // Example threshold for touch point
                // 現在のタッチポイントで近いものがあれば、タッチポイントがそこから移動したとみなす
                bool found = false;
                for (auto& tp: touch_points_) {
                    if (tp.is_close_here(new_tp)) {
                        // 近いものがあれば、タッチポイントを更新する
                        tp.update_touch(new_tp, calc_intensity(new_tp));
                        found = true;
                        break;
                    }
                }
                // 近いものがなければ、新たにタッチポイントを追加する
                if (!found) {
                    new_touch_point(new_tp, calc_intensity(new_tp), midi_callback_);
                }
            }
        }

        // 更新のなかったタッチポイントを削除する
        erase_touch_point();
    }

private:
    auto calc_intensity(size_t pad_num) -> uint16_t {
        uint16_t intensity = 0;
        size_t start = (pad_num > CLOSE_RANGE) ? pad_num - CLOSE_RANGE : 0;
        size_t end = (pad_num + CLOSE_RANGE < MAX_PADS) ? pad_num + CLOSE_RANGE : MAX_PADS - 1;
        for (size_t i = start; i <= end; ++i) {
            intensity += pads_[i].get_crnt();
        } 
        return intensity;
    }
    void new_touch_point(uint16_t location, uint16_t intensity, std::function<void(uint8_t, uint8_t, uint8_t)> callback) {
        for (auto& tp : touch_points_) {
            if (!tp.is_touched()) {
                tp.new_touch(location, intensity, callback);
                return;
            }
        }
    }
    void erase_touch_point() {
        for (auto& tp : touch_points_) {
            if (!tp.is_updated()) {
                tp.released();
            } else {
                tp.clear_updated_flag(); // Clear the updated flag for the next cycle
            }
        }
    }
};
#endif // TOUCH_H