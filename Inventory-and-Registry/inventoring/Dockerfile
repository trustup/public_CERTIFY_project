FROM openjdk:11-jdk

ARG PROJECT_DIR=/app

WORKDIR $PROJECT_DIR

COPY ./ $PROJECT_DIR

RUN chmod +x ./gradlew

CMD ["./gradlew", "clean", "installDist"]
