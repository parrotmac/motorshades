FROM golang:1.18.1-alpine AS build

WORKDIR /app

COPY go.mod go.sum ./

RUN go mod download

COPY main.go ./

ENV CGO_ENABLED=0
RUN go build -o server .

FROM alpine:3.15

RUN apk add --update --no-cache \
    ca-certificates \
    curl

WORKDIR /app

COPY --from=build /app/server ./

CMD ["/app/server"]