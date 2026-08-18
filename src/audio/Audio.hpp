#pragma once
enum class EnemyType;
class Audio { public: bool init(); void playShot(); void playPickup(); void playEnemyAlert(EnemyType); void playEnemyAttack(EnemyType); void playEnemyCharge(EnemyType); private: bool ready_{}; };
