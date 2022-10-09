#include "EntityStructs.hpp"


//-------------------------------------
// Translation Order
//-------------------------------------
bool TranslationOrder::operator==(const TranslationOrder& other) const{
	return matchesWithinTolerance(relative_amounts, other.relative_amounts);
}

//-------------------------------------
// Rotation Order
//-------------------------------------
bool RotationOrder::operator==(const RotationOrder& other) const{
	return matchesWithinTolerance(relative_amounts, other.relative_amounts);
}

//-------------------------------------
// Look Order
//-------------------------------------
bool LookOrder::operator==(const LookOrder& other) const{
	/*
	WARNING: Placeholder implementation. Not bug free.
	*/
	if(type != other.type){
		return false;
	}else{
		switch(type){
			case LOOK_TRACK_POINT:
				return matchesWithinTolerance(target_point, other.target_point);
			case LOOK_TRACK_ENTITY:
				return target_entity == other.target_entity;
			default:
				assert(false);
				return false;
		};
	}
}

//-------------------------------------
// Jump Order
//-------------------------------------
bool JumpOrder::operator==(const JumpOrder& other) const{
	/*
	WARNING: Placeholder implementation. Not bug free.
	*/
	if(type != other.type){
		return false;
	}else{
		switch(type){
			case JUMP_VERTICAL:
				return true;
			case JUMP_TO_POINT_LINEAR:
				return true;
			case JUMP_TO_POINT_PARABOLIC:
				return parabola_height == other.parabola_height;
			default:
				assert(false);
				return false;
		};
	}
}

//-------------------------------------
// Unit Order
//-------------------------------------
bool UnitOrder::operator==(const UnitOrder& other) const{
	if(type != other.type){
		return false;
	}else{
		switch(type){
			case ORDER_ROTATE_ELEMENTWISE:
				return rotation_order == other.rotation_order;
			case ORDER_LOOK_AT:
				return look_order == other.look_order;
			case ORDER_TRANSLATE_CROUCH_WALK:
			case ORDER_TRANSLATE_WALK:
			case ORDER_TRANSLATE_RUN:
				return translation_order == other.translation_order;
			case ORDER_JUMP:
				return jump_order == other.jump_order;
			case ORDER_SHOOT:
				assert(false);
				return false;
			case ORDER_INTERACT:
				assert(false);
				return false;
			default:
				assert(false);
				return false;
		};
	}
}
