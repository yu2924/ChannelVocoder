//
//  CurveMapping.h
//  Fundamental Audio Building Blocks
//
//  Created by yu2924 on 2012-01-20
//  (c) 2012-2021 yu2924
//

#pragma once

#include <cmath>

namespace FABB
{

	template<typename T> class CurveMapLinear
	{
	protected:
		T mAo, mAr, mBo, mBr;
		T mRcpAr, mRcpBr;
	public:
		CurveMapLinear(T al = (T)0, T ah = (T)1, T bl = (T)0, T bh = (T)1) { Setup(al, ah, bl, bh); }
		void Setup(T al, T ah, T bl, T bh) { mAo = al; mAr = ah - al; mBo = bl; mBr = bh - bl; mRcpAr = 1 / mAr; mRcpBr = 1 / mBr; }
		T Map(T v) const { return mBo + (v - mAo) * mBr * mRcpAr; }
		T Unmap(T v) const { return mAo + (v - mBo) * mAr * mRcpBr; }
	};

	template<typename T> class CurveMapExponential
	{
	protected:
		T mAo, mAr, mPw, mSc;
		T mRcpAr, mRcpPw, mRcpSc;
	public:
		CurveMapExponential(T al = (T)0, T ah = (T)1, T bl = (T)0.001, T bh = (T)1) { Setup(al, ah, bl, bh); }
		void Setup(T al, T ah, T bl, T bh) { mAo = al; mAr = ah - al; mPw = std::log(bh) - std::log(bl); mSc = bl; mRcpAr = 1 / mAr; mRcpPw = 1 / mPw; mRcpSc = 1 / mSc; }
		T Map(T v) const { return std::exp((v - mAo) * mRcpAr * mPw) * mSc; }
		T Unmap(T v) const { return mAo + mAr * std::log(v * mRcpSc) * mRcpPw; }
	};

	using CurveMapLinearF = CurveMapLinear<float>;
	using CurveMapLinearD = CurveMapLinear<double>;
	using CurveMapExponentialF = CurveMapExponential<float>;
	using CurveMapExponentialD = CurveMapExponential<double>;

} // namespace FABB
