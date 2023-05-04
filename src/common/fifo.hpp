#pragma once
//=====================================================================//
/*!	@file
	@brief	FIFO (first in first out)
    @author 平松邦仁 (hira@rvf-rc45.net)
	@copyright	Copyright (C) 2016, 2017 Kunihito Hiramatsu @n
				Released under the MIT license @n
				https://github.com/hirakuni45/R8C/blob/master/LICENSE
*/
//=====================================================================//
#include <cstdint>

namespace utils {

    //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
    /*!
        @brief  fifo クラス
		@param[in]	SIZE	バッファサイズ
    */
    //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
	template <typename T, T SIZE>
	class fifo {

		typedef char DT;
		typedef T PTS;

		volatile PTS	get_ = 0;
		volatile PTS	put_ = 0;

		DT	buff_[SIZE];

	public:
        //-----------------------------------------------------------------//
        /*!
            @brief  クリア
        */
        //-----------------------------------------------------------------//
		void clear() { get_ = put_ = 0; }


        //-----------------------------------------------------------------//
        /*!
            @brief  値の格納
			@param[in]	v	値
        */
        //-----------------------------------------------------------------//
		void put(DT v) {
			buff_[put_] = v;
			++put_;
			if(SIZE == 8 || SIZE == 16 || SIZE == 32 || SIZE == 64 || SIZE == 128) {
				put_ &= SIZE - 1;
			} else if(SIZE == 256) {
			} else {
				if(put_ >= SIZE) {
					put_ = 0;
				}
			}
		}


        //-----------------------------------------------------------------//
        /*!
            @brief  値の取得
			@return	値
        */
        //-----------------------------------------------------------------//
		DT get() {
			DT data = buff_[get_];
			++get_;
			if(SIZE == 8 || SIZE == 16 || SIZE == 32 || SIZE == 64 || SIZE == 128) {
				get_ &= SIZE - 1;
			} else if(SIZE == 256) {
			} else {
				if(get_ >= SIZE) {
					get_ = 0;
				}
			}
			return data;
		}


        //-----------------------------------------------------------------//
        /*!
            @brief  長さを返す
			@return	長さ
        */
        //-----------------------------------------------------------------//
		PTS length() const {
			if(put_ >= get_) return (put_ - get_);
			else return (SIZE + put_ - get_);
		}


        //-----------------------------------------------------------------//
        /*!
            @brief  get 位置を返す
			@return	位置
        */
        //-----------------------------------------------------------------//
		PTS pos_get() const { return get_; }


        //-----------------------------------------------------------------//
        /*!
            @brief  put 位置を返す
			@return	位置
        */
        //-----------------------------------------------------------------//
		PTS pos_put() const { return put_; }


        //-----------------------------------------------------------------//
        /*!
            @brief  バッファのサイズを返す
			@return	バッファのサイズ
        */
        //-----------------------------------------------------------------//
		PTS size() const { return SIZE; }
	};

}