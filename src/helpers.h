/**
 * lr(1) helper classes
 * @author Tobias Weber
 * @date 14-jun-2020
 * @license see 'LICENSE.EUPL' file
 */

#ifndef __HELPERS_H__
#define __HELPERS_H__

#include <optional>
#include <iostream>
#include <iomanip>


template<class T=std::size_t, template<class...> class t_cont = std::vector>
class Table
{
public:
	using value_type = T;
	using container_type = t_cont<T>;

	Table() = default;
	~Table() = default;

	Table(const t_cont<t_cont<T>>& cont, T errorval=0xffffffff,
		  std::optional<std::size_t> ROWS=std::nullopt, std::optional<std::size_t> COLS=std::nullopt)
		: m_data{}, m_rowsize{}, m_colsize{}, m_errorval{errorval}
	{
		m_rowsize = ROWS ? *ROWS : cont.size();
		m_colsize = COLS ? *COLS : 0;

		if(!COLS)
		{
			for(const auto& controw : cont)
				m_colsize = std::max(controw.size(), m_colsize);
		}
		m_data.resize(m_rowsize * m_colsize, errorval);

		for(std::size_t row=0; row<m_rowsize; ++row)
		{
			if(row >= cont.size())
				break;

			const auto& controw = cont[row];
			for(std::size_t col=0; col<m_colsize; ++col)
				this->operator()(row, col) = (col < controw.size() ? controw[col] : errorval);
		}
	}

	Table(std::size_t ROWS, std::size_t COLS) : m_data(ROWS*COLS), m_rowsize{ROWS}, m_colsize{COLS} {}

	std::size_t size1() const { return m_rowsize; }
	std::size_t size2() const { return m_colsize; }

	const T& operator()(std::size_t row, std::size_t col) const { return m_data[row*m_colsize + col]; }
	T& operator()(std::size_t row, std::size_t col) { return m_data[row*m_colsize + col]; }

	friend std::ostream& operator<<(std::ostream& ostr, const Table<T, t_cont>& tab)
	{
		const int width = 7;
		for(std::size_t row=0; row<tab.m_rowsize; ++row)
		{
			for(std::size_t col=0; col<tab.m_colsize; ++col)
			{
				T entry = tab(row, col);
				if(entry == tab.m_errorval)
					ostr << std::setw(width) << std::left << "err";
				else
					ostr << std::setw(width) << std::left << entry;
			}
			ostr << "\n";
		}
		return ostr;
	}


private:
	container_type m_data;
	std::size_t m_rowsize, m_colsize;
	T m_errorval = 0;
};


#endif
