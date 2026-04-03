#!/usr/bin/env python3
"""
simulation/scripts/analyze_results.py
Analyze simulation results and generate reports
"""

import os
import sys
import json
import argparse
from pathlib import Path
from typing import Dict, List, Any
import csv
import re


def parse_ros2_bag(bag_path: str) -> Dict[str, List[Dict]]:
    """Parse ros2 bag and extract topic data"""
    # Note: This is a simplified parser
    # In production, use ros2bag Python API
    topics = {}
    
    # For now, return empty dict - would need ros2bag API
    print(f"[INFO] Would parse bag: {bag_path}")
    
    return topics


def load_csv_data(csv_path: str) -> List[Dict]:
    """Load CSV data from ros2 topic echo"""
    data = []
    if not os.path.exists(csv_path):
        return data
    
    with open(csv_path, 'r') as f:
        reader = csv.DictReader(f)
        for row in reader:
            data.append(row)
    
    return data


def calculate_distance(positions: List[Dict]) -> float:
    """Calculate total distance from position data"""
    if len(positions) < 2:
        return 0.0
    
    total = 0.0
    for i in range(1, len(positions)):
        try:
            x1 = float(positions[i-1].get('x', 0))
            y1 = float(positions[i-1].get('y', 0))
            x2 = float(positions[i].get('x', 0))
            y2 = float(positions[i].get('y', 0))
            total += ((x2-x1)**2 + (y2-y1)**2)**0.5
        except (ValueError, KeyError):
            continue
    
    return total


def calculate_speed(positions: List[Dict], time_key: str = 'timestamp') -> float:
    """Calculate average speed"""
    if len(positions) < 2:
        return 0.0
    
    total_dist = calculate_distance(positions)
    # Simplified - would need actual timestamps
    return total_dist / max(len(positions), 1)


def detect_rendezvous(truck_positions: List[Dict], drone_positions: List[Dict], 
                      threshold_dist: float = 2.0, threshold_time: float = 5.0) -> Dict[str, Any]:
    """Detect rendezvous events between truck and drones"""
    rendezvous = []
    
    # Simple proximity detection
    for i, drone_pos in enumerate(drone_positions):
        for j, truck_pos in enumerate(truck_positions):
            try:
                dx = float(drone_pos.get('x', 0)) - float(truck_pos.get('x', 0))
                dy = float(drone_pos.get('y', 0)) - float(truck_pos.get('y', 0))
                dz = float(drone_pos.get('z', 0)) - float(truck_pos.get('z', 0))
                dist = (dx**2 + dy**2 + dz**2)**0.5
                
                if dist < threshold_dist:
                    rendezvous.append({
                        'drone_idx': i,
                        'truck_idx': j,
                        'distance': dist,
                        'drone_position': (dx, dy, dz)
                    })
            except (ValueError, KeyError):
                continue
    
    return {
        'count': len(rendezvous),
        'events': rendezvous,
        'success_rate': len(rendezvous) / max(len(drone_positions), 1)
    }


def calculate_battery_stats(battery_data: List[Dict]) -> Dict[str, float]:
    """Calculate battery statistics"""
    if not battery_data:
        return {'min': 0, 'max': 0, 'avg': 0, 'consumption': 0}
    
    levels = []
    for row in battery_data:
        try:
            level = float(row.get('percentage', row.get('level', 0)))
            levels.append(level)
        except (ValueError, KeyError):
            continue
    
    if not levels:
        return {'min': 0, 'max': 0, 'avg': 0, 'consumption': 0}
    
    return {
        'min': min(levels),
        'max': max(levels),
        'avg': sum(levels) / len(levels),
        'consumption': max(levels) - min(levels)
    }


def analyze_simulation(results_dir: str) -> Dict[str, Any]:
    """Main analysis function"""
    results = {
        'simulation_dir': results_dir,
        'truck': {},
        'drone': {},
        'rendezvous': {},
        'battery': {}
    }
    
    results_path = Path(results_dir)
    
    # Load truck data
    truck_odom = results_path / 'truck_odom.csv'
    if truck_odom.exists():
        truck_data = load_csv_data(str(truck_odom))
        results['truck'] = {
            'distance': calculate_distance(truck_data),
            'avg_speed': calculate_speed(truck_data),
            'data_points': len(truck_data)
        }
    
    # Load drone data
    drone_pose = results_path / 'drone_pose.csv'
    if drone_pose.exists():
        drone_data = load_csv_data(str(drone_pose))
        results['drone'] = {
            'distance': calculate_distance(drone_data),
            'avg_speed': calculate_speed(drone_data),
            'data_points': len(drone_data)
        }
        
        # Calculate rendezvous
        if truck_data and drone_data:
            results['rendezvous'] = detect_rendezvous(truck_data, drone_data)
    
    # Load battery data
    drone_battery = results_path / 'drone_battery.csv'
    if drone_battery.exists():
        battery_data = load_csv_data(str(drone_battery))
        results['battery'] = calculate_battery_stats(battery_data)
    
    return results


def print_results(results: Dict[str, Any]) -> None:
    """Print analysis results"""
    print("\n" + "="*50)
    print("SIMULATION ANALYSIS RESULTS")
    print("="*50)
    
    print("\n🚛 TRUCK:")
    truck = results.get('truck', {})
    print(f"  Distance: {truck.get('distance', 0):.2f} m")
    print(f"  Avg Speed: {truck.get('avg_speed', 0):.2f} m/s")
    print(f"  Data Points: {truck.get('data_points', 0)}")
    
    print("\n🚁 DRONE:")
    drone = results.get('drone', {})
    print(f"  Distance: {drone.get('distance', 0):.2f} m")
    print(f"  Avg Speed: {drone.get('avg_speed', 0):.2f} m/s")
    print(f"  Data Points: {drone.get('data_points', 0)}")
    
    print("\n🤝 RENDEZVOUS:")
    rv = results.get('rendezvous', {})
    print(f"  Count: {rv.get('count', 0)}")
    print(f"  Success Rate: {rv.get('success_rate', 0)*100:.1f}%")
    
    print("\n🔋 BATTERY:")
    battery = results.get('battery', {})
    print(f"  Min: {battery.get('min', 0):.1f}%")
    print(f"  Max: {battery.get('max', 0):.1f}%")
    print(f"  Avg: {battery.get('avg', 0):.1f}%")
    print(f"  Consumption: {battery.get('consumption', 0):.1f}%")
    
    print("\n" + "="*50)


def main():
    parser = argparse.ArgumentParser(description='Analyze OpenClaw simulation results')
    parser.add_argument('results_dir', nargs='?', default='results',
                       help='Path to results directory')
    parser.add_argument('--json', action='store_true', help='Output JSON')
    parser.add_argument('--output', help='Output file for results')
    
    args = parser.parse_args()
    
    # Find results directory
    script_dir = Path(__file__).parent.parent
    results_dir = script_dir / args.results_dir
    
    if not results_dir.exists():
        print(f"[ERROR] Results directory not found: {results_dir}")
        sys.exit(1)
    
    print(f"[INFO] Analyzing: {results_dir}")
    
    # Analyze
    results = analyze_simulation(str(results_dir))
    
    # Print
    if args.json:
        print(json.dumps(results, indent=2))
    else:
        print_results(results)
    
    # Save to file if requested
    if args.output:
        with open(args.output, 'w') as f:
            json.dump(results, f, indent=2)
        print(f"\n[INFO] Results saved to: {args.output}")
    
    return 0


if __name__ == '__main__':
    sys.exit(main())