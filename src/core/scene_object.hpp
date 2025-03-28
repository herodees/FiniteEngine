#pragma once

#include "include.hpp"
#include "utils/spatial.hpp"

namespace fin
{
	class Renderer;

	enum SceneObjectFlag
	{
		Marked = 1 << 0,
		Disabled = 1 << 1,
		Hidden = 1 << 2,
	};

	class SceneObject : public SpatialItem
	{
		friend class scene;
	public:
		inline static std::string_view type_id = "scno";

		SceneObject() = default;
		virtual ~SceneObject() = default;

		virtual void render(Renderer& dc) = 0;
		virtual void serialize(msg::Writer& ar);
		virtual void deserialize(msg::Value& ar);
		virtual std::string_view object_type() { return type_id; };

		bool is_below(const SceneObject& rt) const;

		int32_t depth_get() const;
		void depth_reset();

		bool flag_get(SceneObjectFlag f) const;
		void flag_reset(SceneObjectFlag f);
		void flag_set(SceneObjectFlag f);

		bool is_disabled() const { return flag_get(SceneObjectFlag::Disabled); }
		void disable(bool v) { v ? flag_set(SceneObjectFlag::Disabled) : flag_reset(SceneObjectFlag::Disabled); }
		
		bool is_hidden() const { return flag_get(SceneObjectFlag::Hidden); }
		void hide(bool v) { v ? flag_set(SceneObjectFlag::Hidden) : flag_reset(SceneObjectFlag::Hidden); }

	protected:

		mutable uint32_t _id{};
		mutable int32_t _iso_depth{};
		Line<float> _iso_line;
		Vec2f _iso_a;
		Vec2f _iso_b;
		std::vector<SceneObject*> _objects_behind;
	};



	inline void SceneObject::serialize(msg::Writer& ar)
	{
		ar.member("x", _bbox.x1);
		ar.member("y", _bbox.y1);
		ar.member("fl", _flag);
	}

	inline void SceneObject::deserialize(msg::Value& ar)
	{
		_bbox.x1 = ar["x"].get(_bbox.x1);
		_bbox.y1 = ar["y"].get(_bbox.y1);
		_flag = ar["fl"].get(_flag);
	}

	
	inline bool SceneObject::is_below(const SceneObject& rt) const
	{
		return _iso_line.compare(rt._iso_line) > 0;


		// Compute the cross product (2D determinant method)
		const float position = (_bbox.x1 + _iso_b.x - (_bbox.x1 + _iso_a.x)) * (rt._bbox.y1 + rt._iso_a.y - (_bbox.y1 + _iso_a.y)) -
			(_bbox.y1 + _iso_b.y - (_bbox.y1 + _iso_a.y)) * (rt._bbox.x1 + rt._iso_a.x - (_bbox.x1 + _iso_a.x));
		return position > 0;
	}
	

	inline int32_t SceneObject::depth_get() const
	{
		if (!_iso_depth)
		{
			if (_objects_behind.empty())
				_iso_depth = 1;
			else
			{
				for (auto el : _objects_behind)
					_iso_depth = std::max(el->depth_get(), _iso_depth);
				_iso_depth += 1;
			}
		}
		return _iso_depth;
	}

	inline void SceneObject::depth_reset()
	{
		_iso_depth = 0;
		_iso_line.point1.x = _iso_a.x + _bbox.x1;
		_iso_line.point2.x = _iso_b.x + _bbox.x1;
		_iso_line.point1.y = _iso_a.y + _bbox.y1;
		_iso_line.point2.y = _iso_b.y + _bbox.y1;
		_objects_behind.clear();
	}

	inline bool SceneObject::flag_get(SceneObjectFlag f) const
	{
		return _flag & f;
	}

	inline void SceneObject::flag_reset(SceneObjectFlag f)
	{
		_flag &= ~f;
	}

	inline void SceneObject::flag_set(SceneObjectFlag f)
	{
		_flag |= f;
	}
}