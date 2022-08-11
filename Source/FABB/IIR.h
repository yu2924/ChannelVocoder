//
//  IIR.h
//  Fundamental Audio Building Blocks
//
//  Created by yu2924 on 2012-01-09
//  (c) 2012-2015 yu2924
//

#pragma once

namespace FABB
{

	//==============================================================================
	// Direct-Form I IIRs

	namespace DirectFormI
	{

		// Direct-Form I
		// 
		//             b0
		// x[n] ----+--|>--|+|------+----y[n]
		//          |       |       |
		//        |z-1|     |     |z-1|
		//          |  b1   |  -a1  |
		//          +--|>--|+|--<|--+
		// 
		//        b0 + b1*z^-1   b0*z + b1
		// H(z) = ------------ = ---------
		//         1 + a1*z~-1      z + a1
		// 
		// y[n] = b0*x[n] + b1*x[n-1] - a1*y[n-1]
		template<typename T> class IIR1T
		{
		public:
			using FloatType = T;
			struct Coef { T a1, b0, b1; };
			Coef mCoef;
			T mX1, mY1;
			IIR1T()
			{
				mCoef.b0 = 1; mCoef.b1 = mCoef.a1 = 0;
				Reset();
			}
			void GetCoefficients(Coef* p) const
			{
				*p = mCoef;
			}
			void SetCoefficients(const Coef& v)
			{
				mCoef = v;
			}
			void CopyCoefficients(const IIR1T<T>& r)
			{
				mCoef = r.mCoef;
			}
			void Reset()
			{
				mX1 = mY1 = 0;
			}
			T Process(T x)
			{
				T y = mCoef.b0 * x + mCoef.b1 * mX1 - mCoef.a1 * mY1;
				mX1 = x;
				mY1 = y;
				return y;
			}
			// allows inplace (pd==ps)
			void Process(const T* ps, T* pd, size_t l)
			{
				while(l --) *pd ++ = Process(*ps ++);
			}
			void Process(T* p, size_t l)
			{
				Process(p, p, l);
			}
		};

		// Direct-Form I
		// 
		//             b0
		// x[n] ----+--|>--|+|------+----y[n]
		//          |       |       |
		//        |z-1|     |     |z-1|
		//          |  b1   |  -a1  |
		//          +--|>--|+|--<|--+
		//          |       |       |
		//        |z-1|     |     |z-1|
		//          |  b2   |  -a2  |
		//          +--|>---+---<|--+
		// 
		//        b0 + b1*z^-1 + b2*z^-2   b0*Z^2 + b1*z + b2
		// H(z) = ---------------------- = ------------------
		//         1 + a1*z^-1 + a2*z^-2      Z^2 + a1*z + a2
		// 
		// y[n] = b0*x[n] + b1*x[n-1] + b2*x[n-2] - a1*y[n-1] - a2*y[n-2]
		template<typename T> class IIR2T
		{
		public:
			using FloatType = T;
			struct Coef { T a1, a2, b0, b1, b2; };
			Coef mCoef;
			T mX1, mX2, mY1, mY2;
			IIR2T()
			{
				mCoef.b0 = 1; mCoef.b1 = mCoef.b2 = mCoef.a1 = mCoef.a2 = 0;
				Reset();
			}
			void GetCoefficients(Coef* p) const
			{
				*p = mCoef;
			}
			void SetCoefficients(const Coef& v)
			{
				mCoef = v;
			}
			void CopyCoefficients(const IIR2T<T>& r)
			{
				mCoef = r.mCoef;
			}
			void Reset()
			{
				mX1 = mX2 = mY1 = mY2 = 0;
			}
			T Process(T x)
			{
				T y = mCoef.b0 * x + mCoef.b1 * mX1 + mCoef.b2 * mX2 - mCoef.a1 * mY1 - mCoef.a2 * mY2;
				mX2 = mX1; mX1 = x;
				mY2 = mY1; mY1 = y;
				return y;
			}
			// allows inplace (pd==ps)
			void Process(const T* ps, T* pd, size_t l)
			{
				while(l --) *pd ++ = Process(*ps ++);
			}
			void Process(T* p, size_t l)
			{
				Process(p, p, l);
			}
		};

		// Direct-Form I
		// 
		//             b0
		// x[n] ----+--|>--|+|------+----y[n]
		//          |       |       |
		//        |z-1|     |     |z-1|
		//          |  b1   |  -a1  |
		//          +--|>--|+|--<|--+
		//          |       |       |
		//        |z-1|     |     |z-1|
		//          |  b2   |  -a2  |
		//          +--|>---+---<|--+
		//          |       |       |
		//        |z-1|     |     |z-1|
		//          |  b3   |  -a3  |
		//          +--|>---+---<|--+
		//          |       |       |
		//        |z-1|     |     |z-1|
		//          |  b4   |  -a4  |
		//          +--|>---+---<|--+
		// 
		//        b0 + b1*z^-1 + b2*z^-2 + b3*z^-3 + b4*z^-4   b0*z^4 + b1*z^3 + b2*z^2 + b3*z + b4
		// H(z) = ------------------------------------------ = ------------------------------------
		//         1 + a1*z^-1 + a2*z^-2 + a3*z^-3 + a4*z^-4      Z^4 + a1*z^3 + a2*z^2 + a3*z + a4
		// 
		// y[n] = b0*x[n] + b1*x[n-1] + b2*x[n-2] + b3*x[n-3] + b4*x[n-4] - a1*y[n-1] - a2*y[n-2] - a3*y[n-3] - a4*y[n-4]
		template<typename T> class IIR4T
		{
		public:
			using FloatType = T;
			struct Coef { T a1, a2, a3, a4, b0, b1, b2, b3, b4; };
			Coef mCoef;
			T mX1, mX2, mX3, mX4, mY1, mY2, mY3, mY4;
			IIR4T()
			{
				mCoef.b0 = 1; mCoef.b1 = mCoef.b2 = mCoef.b3 = mCoef.b4 = mCoef.a1 = mCoef.a2 = mCoef.a3 = mCoef.a4 = 0;
				Reset();
			}
			void GetCoefficients(Coef* p) const
			{
				*p = mCoef;
			}
			void SetCoefficients(const Coef& v)
			{
				mCoef = v;
			}
			void CopyCoefficients(const IIR4T<T>& r)
			{
				mCoef = r.mCoef;
			}
			void Reset()
			{
				mX1 = mX2 = mX3 = mX4 = mY1 = mY2 = mY3 = mY4 = 0;
			}
			T Process(T x)
			{
				T y = mCoef.b0 * x + mCoef.b1 * mX1 + mCoef.b2 * mX2 + mCoef.b3 * mX3 + mCoef.b4 * mX4 - mCoef.a1 * mY1 - mCoef.a2 * mY2 - mCoef.a3 * mY3 - mCoef.a4 * mY4;
				mX4 = mX3; mX3 = mX2; mX2 = mX1; mX1 = x;
				mY4 = mY3; mY3 = mY2; mY2 = mY1; mY1 = y;
				return y;
			}
			// allows inplace (pd==ps)
			void Process(const T* ps, T* pd, size_t l)
			{
				while(l --) *pd ++ = Process(*ps ++);
			}
			void Process(T* p, size_t l)
			{
				Process(p, p, l);
			}
		};

	} // namespace DirectFormI

	//==============================================================================
	// Transposed Direct-Form II IIRs

	namespace TransposedDirectFormII
	{

		// Transposed Direct-Form II
		// 
		// 	        b0
		// x[n] ----+--|>--|+|------+----y[n]
		//          |       | s1    |
		//          |     |z-1|     |
		//          |  b1   |  -a1  |
		//          +--|>--|+|--<|--+
		// 
		//        b0 + b1*z^-1   b0*z + b1
		// H(z) = ------------ = ---------
		//         1 + a1*z~-1      z + a1
		// 
		// y = b0*x + s1; s1 = b1*x - a1*y;
		template<typename T> class IIR1T
		{
		public:
			using FloatType = T;
			struct Coef { T a1, b0, b1; };
			Coef mCoef;
			T mS1;
			IIR1T()
			{
				mCoef.b0 = 1; mCoef.b1 = mCoef.a1 = 0;
				Reset();
			}
			void GetCoefficients(Coef* p) const
			{
				*p = mCoef;
			}
			void SetCoefficients(const Coef& v)
			{
				mCoef = v;
			}
			void CopyCoefficients(const IIR1T<T>& r)
			{
				mCoef = r.mCoef;
			}
			void Reset()
			{
				mS1 = 0;
			}
			T Process(T x)
			{
				T y = mCoef.b0*x + mS1;
				mS1 = mCoef.b1*x - mCoef.a1*y;
				return y;
			}
			// allows inplace (pd==ps)
			void Process(const T* ps, T* pd, size_t l)
			{
				while(l --) *pd ++ = Process(*ps ++);
			}
			void Process(T* p, size_t l)
			{
				Process(p, p, l);
			}
		};

		// Transposed Direct-Form II
		// 
		//             b0
		// x[n] ----+--|>--|+|------+----y[n]
		//          |       | s1    |
		//          |     |z-1|     |
		//          |  b1   |  -a1  |
		//          +--|>--|+|--<|--+
		//          |       | s2    |
		//          |     |z-1|     |
		//          |  b2   |  -a2  |
		//          +--|>--|+|--<|--+
		// 
		//        b0 + b1*z^-1 + b2*z^-2   b0*Z^2 + b1*z + b2
		// H(z) = ---------------------- = ------------------
		//         1 + a1*z^-1 + a2*z^-2      Z^2 + a1*z + a2
		// 
		// y = b0*x + s1; s1 = b1*x - a1*y + s2; s2 = b2*x - a2*y;
		template<typename T> class IIR2T
		{
		public:
			using FloatType = T;
			struct Coef { T a1, a2, b0, b1, b2; };
			Coef mCoef;
			T mS1, mS2;
			IIR2T()
			{
				mCoef.b0 = 1; mCoef.b1 = mCoef.b2 = mCoef.a1 = mCoef.a2 = 0;
				Reset();
			}
			void GetCoefficients(Coef* p) const
			{
				*p = mCoef;
			}
			void SetCoefficients(const Coef& v)
			{
				mCoef = v;
			}
			void CopyCoefficients(const IIR2T<T>& r)
			{
				mCoef = r.mCoef;
			}
			void Reset()
			{
				mS1 = mS2 = 0;
			}
			T Process(T x)
			{
				T y = mCoef.b0*x + mS1;
				mS1 = mCoef.b1*x - mCoef.a1*y + mS2;
				mS2 = mCoef.b2*x - mCoef.a2*y;
				return y;
			}
			// allows inplace (pd==ps)
			void Process(const T* ps, T* pd, size_t l)
			{
				while(l --) *pd ++ = Process(*ps ++);
			}
			void Process(T* p, size_t l)
			{
				Process(p, p, l);
			}
		};

		// Transposed Direct-Form II
		// 
		//             b0
		// x[n] ----+--|>--|+|------+----y[n]
		//          |       | s1    |
		//          |     |z-1|     |
		//          |  b1   |  -a1  |
		//          +--|>--|+|--<|--+
		//          |       | s2    |
		//          |     |z-1|     |
		//          |  b2   |  -a2  |
		//          +--|>--|+|--<|--+
		//          |       | s3    |
		//          |     |z-1|     |
		//          |  b3   |  -a3  |
		//          +--|>--|+|--<|--+
		//          |       | s4    |
		//          |     |z-1|     |
		//          |  b4   |  -a4  |
		//          +--|>--|+|--<|--+
		// 
		//        b0 + b1*z^-1 + b2*z^-2 + b3*z^-3 + b4*z^-4   b0*z^4 + b1*z^3 + b2*z^2 + b3*z + b4
		// H(z) = ------------------------------------------ = ------------------------------------
		//         1 + a1*z^-1 + a2*z^-2 + a3*z^-3 + a4*z^-4      Z^4 + a1*z^3 + a2*z^2 + a3*z + a4
		// 
		// y = b0*x + s1; s1 = b1*x - a1*y + s2; s2 = b2*x - a2*y + s3; s3 = b3*x - a3*y + s4; s4 = b4*x - a4*y;
		template<typename T> class IIR4T
		{
		public:
			using FloatType = T;
			struct Coef { T a1, a2, a3, a4, b0, b1, b2, b3, b4; };
			Coef mCoef;
			T mS1, mS2, mS3, mS4;
			IIR4T()
			{
				mCoef.b0 = 1; mCoef.b1 = mCoef.b2 = mCoef.b3 = mCoef.b4 = mCoef.a1 = mCoef.a2 = mCoef.a3 = mCoef.a4 = 0;
				Reset();
			}
			void GetCoefficients(Coef* p) const
			{
				*p = mCoef;
			}
			void SetCoefficients(const Coef& v)
			{
				mCoef = v;
			}
			void CopyCoefficients(const IIR4T<T>& r)
			{
				mCoef = r.mCoef;
			}
			void Reset()
			{
				mS1 = mS2 = mS3 = mS4 = 0;
			}
			T Process(T x)
			{
				T y = mCoef.b0*x + mS1;
				mS1 = mCoef.b1*x - mCoef.a1*y + mS2;
				mS2 = mCoef.b2*x - mCoef.a2*y + mS3;
				mS3 = mCoef.b3*x - mCoef.a3*y + mS4;
				mS4 = mCoef.b4*x - mCoef.a4*y;
				return y;
			}
			// allows inplace (pd==ps)
			void Process(const T* ps, T* pd, size_t l)
			{
				while(l --) *pd ++ = Process(*ps ++);
			}
			void Process(T* p, size_t l)
			{
				Process(p, p, l);
			}
		};

	} // namespace TransposedDirectFormII

	//==============================================================================
	// aliases

#if defined(FABB_IIR_DEFAULT_DIRECTFORMI) && FABB_IIR_DEFAULT_DIRECTFORMI
	template<typename T> using IIR1T = DirectFormI::IIR1T<T>;
	template<typename T> using IIR2T = DirectFormI::IIR2T<T>;
	template<typename T> using IIR4T = DirectFormI::IIR4T<T>;
#else
	template<typename T> using IIR1T = TransposedDirectFormII::IIR1T<T>;
	template<typename T> using IIR2T = TransposedDirectFormII::IIR2T<T>;
	template<typename T> using IIR4T = TransposedDirectFormII::IIR4T<T>;
#endif

	using IIR1F = IIR1T<float>;
	using IIR1D = IIR1T<double>;
	using IIR2F = IIR2T<float>;
	using IIR2D = IIR2T<double>;
	using IIR4F = IIR4T<float>;
	using IIR4D = IIR4T<double>;

} // namespace FABB
