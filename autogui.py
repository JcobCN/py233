import pyautogui,time

agui = pyautogui

class Paint():
    def drawRoung(self):
        time.sleep(5)

        pyautogui.click()
        distance = 200
        while distance > 0:
            agui.dragRel(distance, 0, duration=0.2)
            distance = distance - 5
            agui.dragRel(0, distance, duration=0.2)
            agui.dragRel(-distance, 0, duration=0.2)
            distance = distance - 5
            agui.dragRel(0, -distance, duration=0.2)


if __name__ == '__main__':
    paintOnWin = Paint()
    paintOnWin.drawRoung()