#!/usr/bin/env python3
"""
按钮状态变化可视化脚本
展示右ABC按钮每隔1秒交替按下和弹起的效果
"""

import time
import os

def clear_screen():
    """清屏"""
    os.system('cls' if os.name == 'nt' else 'clear')

def show_button_status(seconds, pressed):
    """显示按钮状态"""
    clear_screen()
    
    status = "按下" if pressed else "弹起"
    buttons = "ABC" if pressed else "   "
    
    print("右ABC按钮状态可视化")
    print("=" * 30)
    print(f"时间: {seconds}秒")
    print(f"状态: {status}")
    print()
    print("右ABC按钮:")
    print(f"  [{buttons}]")
    print()
    print("数据包:")
    if pressed:
        print("  44 44 54 00 07 00 40")
    else:
        print("  44 44 54 00 00 00 40")
    print()
    print("按 Ctrl+C 停止")

def animate_button_changes():
    """动画显示按钮状态变化"""
    try:
        seconds = 0
        while True:
            pressed = (seconds % 2) == 0
            show_button_status(seconds, pressed)
            time.sleep(1)
            seconds += 1
    except KeyboardInterrupt:
        print("\n动画停止")

def main():
    """主函数"""
    print("右ABC按钮状态变化动画")
    print("=" * 30)
    print("每隔1秒交替按下和弹起")
    print("按 Enter 开始动画...")
    input()
    
    animate_button_changes()

if __name__ == "__main__":
    main() 