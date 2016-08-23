#pragma once

#ifndef _CHARACTERCONTROLLER_ACTIVITIES_HPP_
#define _CHARACTERCONTROLLER_ACTIVITIES_HPP_

#include <ai/CharacterController.hpp>


class CharacterObject;
class VehicleObject;
class WeaponItem;


namespace Activities {
	class GoTo : public CharacterController::Activity
	{
	public:
		bool update(CharacterObject*, CharacterController*) override;
		bool canSkip(CharacterObject *) const override { return true; }

		GoTo(const glm::vec3& target, bool sprint=false) :
			Activity(Type::GoTo), target(target), sprint(sprint) { }

	private:
		glm::vec3 target;
		bool sprint;
	};


	class Jump : public CharacterController::Activity
	{
	public:
		bool update(CharacterObject*, CharacterController*) override;

		Jump() : Activity(Type::Jump), jumped(false) { }

	private:
		bool jumped;
	};


	class EnterVehicle : public CharacterController::Activity
	{
	public:
		enum {
			ANY_SEAT = -1 // Magic number for any seat but the driver's.
		};

		bool update(CharacterObject*, CharacterController*) override;
		bool canSkip(CharacterObject *) const override;

		EnterVehicle(VehicleObject* vehicle, int seat=0) :
			Activity(Type::EnterVehicle), vehicle(vehicle),
			seat(seat), entering(false) { }

	private:
		VehicleObject* vehicle;
		int seat;
		bool entering;
	};


	class ExitVehicle : public CharacterController::Activity
	{
	public:
		bool update(CharacterObject*, CharacterController*) override;

		ExitVehicle(bool jacked=false) :
			Activity(Type::ExitVehicle), jacked(jacked) { }

	private:
		const bool jacked;
	};


	class ShootWeapon : public CharacterController::Activity
	{
	public:
		bool update(CharacterObject*, CharacterController*) override;

		ShootWeapon(WeaponItem* item) :
			Activity(Type::ShootWeapon), item(item), fired(false) { }

	private:
		WeaponItem* item;
		bool fired;
	};
}

#endif
