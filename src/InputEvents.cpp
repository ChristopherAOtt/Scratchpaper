#include "InputEvents.hpp"

bool operator==(InputEvent e1, InputEvent e2){
	return memcmp(&e1, &e2, sizeof(InputEvent));
	/*
	if(e1.source_type != e2.source_type){
		return false;
	}else{
		switch e1.source_type{
			case DEVICE_INVALID
		}
	}
	*/
}
