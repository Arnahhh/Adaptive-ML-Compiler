public class Main {
    public static void main(String[] args) {
        int x = 10;
        int y = 20;
        if (x < y) {
            x = x + 1;
        } else {
            x = x + 2;
        }
        while (x < 15) {
            x = x + 1;
        }
        return x;
    }
}
