#pragma once

#include "math_utils.hpp"
#include "include.hpp"

namespace fin
{
	struct SpatialItem
	{
		Region<float> _bbox;
		mutable Region<int32_t> _grid;
		mutable uint32_t _flag{};

		Region<int32_t> get_grid(int32_t cell_size) const
		{
			return Region<int32_t>(_bbox.x1 / cell_size, _bbox.y1 / cell_size,
				_bbox.x2 / cell_size, _bbox.y2 / cell_size);
		}
	};


	class SpatialGrid
	{
	public:
		int32_t _grid_width;
		int32_t _grid_height;
		int32_t _cell_size;
		int32_t _grid_stride;
		std::vector<std::vector<SpatialItem*>> _grid;

		SpatialGrid(int32_t width, int32_t height, int32_t cellSize)
			: _grid_width(width / cellSize),
			_grid_height(height / cellSize),
			_cell_size(cellSize),
			_grid_stride(width / cellSize),
			_grid(_grid_width * _grid_height)
		{
		}

		void clear(int32_t width, int32_t height, int32_t cellSize)
		{
			_grid_width = (width / cellSize);
			_grid_height = (height / cellSize);
			_cell_size = (cellSize);
			_grid_stride = (width / cellSize);
			_grid.resize(_grid_width * _grid_height);
		}

		void insert(SpatialItem* r)
		{
			r->_grid = r->get_grid(_cell_size);

			for (int32_t x = r->_grid.x1; x <= r->_grid.x2; ++x)
			{
				for (int32_t y = r->_grid.y1; y <= r->_grid.y2; ++y)
				{
					_grid[x + _grid_stride * y].push_back(r);
				}
			}
		}

		void remove(SpatialItem* r)
		{
			for (int32_t x = r->_grid.x1; x <= r->_grid.x2; ++x)
			{
				for (int32_t y = r->_grid.y1; y <= r->_grid.y2; ++y)
				{
					auto& cell = _grid[x + _grid_stride * y];
					for (size_t i = 0; i < cell.size(); ++i)
					{
						if (cell[i] == r)
						{
							std::swap(cell[i], cell.back()); // Swap with last element
							cell.pop_back(); // Remove last element (O(1))
							break;
						}
					}
				}
			}
		}

		size_t query(const Region<float>& queryRect, std::vector<SpatialItem*>& result, bool clear_result = true) const
		{
			if (clear_result)
				result.clear();

			const size_t from = result.size();

			Region<int32_t> reg(queryRect.x1 / _cell_size,
				queryRect.y1 / _cell_size,
				queryRect.x2 / _cell_size,
				queryRect.y2 / _cell_size);

			reg.x2 = std::min(reg.x2, _grid_width - 1);
			reg.y2 = std::min(reg.y2, _grid_height - 1);

			for (int32_t x = reg.x1; x <= reg.x2; ++x)
			{
				for (int32_t y = reg.y1; y <= reg.y2; ++y)
				{
					int32_t index = x + _grid_stride * y;
					for (SpatialItem* r : _grid[index])
					{
						if (!(r->_flag & 1) && reg.intersects(r->_grid))
						{
							r->_flag |= 1;
							result.push_back(r);
						}
					}
				}
			}

			std::for_each(result.begin() + from, result.end(), [](SpatialItem* c) { c->_flag &= ~1u; });

			return result.size();
		}

		std::vector<SpatialItem*> query(const Region<float>& queryRect) const
		{
			std::vector<SpatialItem*> result;
			query(queryRect, result);
			return result;
		}
	};
}
