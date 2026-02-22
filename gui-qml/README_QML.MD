# Install prerequisites (Ubuntu)
sudo apt update
sudo apt install build-essential cmake qt6-base-dev qt6-declarative-dev

# Install prerequisites (Arch)
sudo pacman -S base-devel cmake qt6-base qt6-declarative

# From repo root
mkdir -p build
cd build
cmake -S .. -B .
cmake --build . -j$(nproc)

# Run the app
./gui-qml/kotoba-qml